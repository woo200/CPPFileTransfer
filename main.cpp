#include "main.hpp"

#define CHUNK_SIZE 128000

void send_command(woo200::ClientSocket &sock, char command)
{
    woo200::PCommand cmd(command);
    cmd.send_to_socket(sock);
}

char recv_command(woo200::ClientSocket &sock)
{
    woo200::PCommand cmd;
    if (cmd.read_from_socket(sock) < 0) {
        fprintf(stderr, "Failed to receive command\n");
        exit(EXIT_FAILURE);
    }
    return cmd.get_command();
}

int count(std::string str, char c)
{
    int count = 0;
    for (char ch : str) {
        if (ch == c)
            count++;
    }
    return count;
}

std::string base_name(std::string path, int deepness = 0)
{
    bool has_ending_slash = path[path.length()-1] == '/';
    if (!has_ending_slash)
        path += "/";

    int num_slashes = count(path, '/') - 1;
    std::istringstream f(path);
    std::string s, basename;
    int i = 0;

    while (getline(f, s, '/')) {
        if (num_slashes-i <= deepness) {
            basename += s + "/";
        }
        i++;
    }

    if (!has_ending_slash)
        return basename.substr(0, basename.length()-1);

    return basename;
}

void create_dirs(std::string file_path)
{
    bool last_file_is_dir = file_path[file_path.length()-1] == '/';
    int num_slashes = count(file_path, '/');

    std::istringstream f(file_path);
    std::string s, dir_path;
    struct stat info;

    int i = 0;
    while (getline(f, s, '/')) {
        dir_path += s + "/";
        if (num_slashes-i == 0) {
            break;
        }
        i++;
        if(stat( dir_path.c_str(), &info ) == 0)
            continue;

        mkdir(dir_path.c_str(), 0777);
    }
}

// ################################
// ################################
// ############ SERVER ############
// ################################
// ################################

int receive_file(woo200::ClientSocket &client)
{
    woo200::PFileHeader header;
    header.read_from_socket(client);

    std::string filename = header.get_filename();
    unsigned long size = header.get_filesize();

    printf("Receiving file %s (%lu bytes)\n", filename.c_str(), size);

    create_dirs(filename); // Create directories if they don't exist

    FILE* fp = fopen(filename.c_str(), "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file %s\n", filename.c_str());
        send_command(client, 'e'); // Error
        return -1;
    }

    send_command(client, 'r'); // Ready

    char buf[CHUNK_SIZE];
    int read_len;
    while (size > 0 && (read_len = client.recv(buf, CHUNK_SIZE)) > 0) {
        fwrite(buf, 1, read_len, fp);
        size -= read_len;
    }

    send_command(client, 'c'); // Complete

    fclose(fp);
    return 1;
}

void start_receive(std::string address, int port)
{
    woo200::ServerSocket sock;
    if (sock.bind(address.c_str(), port) < 0) {
        fprintf(stderr, "Failed to bind to %s:%d\n", address.c_str(), port);
        sock.close();
        exit(EXIT_FAILURE);
    }
    if (sock.listen(5) < 0) {
        fprintf(stderr, "Failed to listen on port %s:%d\n", address.c_str(), port);
        sock.close();
        exit(EXIT_FAILURE);
    }
    std::cout << "Listening on " << address << ":" << port << "\n";
    woo200::ClientSocket* client = sock.accept();
    std::cout << "Accepted connection from " << inet_ntoa(client->get_addr().sin_addr) << "\n";

    char cmd;
    do {
        cmd = recv_command(*client);

        switch (cmd) {
            case 'f':
                receive_file(*client);
                break;
            case 'd':
                std::cout << "Client softly disconnected\n";
                break;
            default:
                fprintf(stderr, "Unknown command: %c\n", cmd);
                break;
        }
    } while (cmd != 'd');

    client->close();
}

// ################################
// ################################
// ############ CLIENT ############
// ################################
// ################################
// ################################

int send_file(std::string file_path, woo200::ClientSocket &sock, int deepness)
{
    FILE* fp = fopen(file_path.c_str(), "r");
    if (fp == NULL) 
        return -1;
    
    // Get file length
    fseek(fp, 0, SEEK_END);
    unsigned long file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Tell server it's receiving a file
    send_command(sock, 'f');

    // Send file header packet
    std::string basename = base_name(file_path, deepness);
    woo200::PFileHeader header(basename, file_len);
    header.send_to_socket(sock);

    // Wait for server to be ready
    char cmd = recv_command(sock);
    if (cmd != 'r') {
        fprintf(stderr, "Server not ready to receive file\n");
        return -1;
    }

    // Send file data
    char buf[CHUNK_SIZE];
    int read_len;
    while ((read_len = fread(buf, 1, CHUNK_SIZE, fp)) > 0) {
        sock.send(buf, read_len);
    }
    fclose(fp);

    // Wait for server to be done
    cmd = recv_command(sock);

    return 0;
}

void send_file(std::string address, int port, const char* filename)
{
    woo200::ClientSocket* sock = new woo200::ClientSocket();
    if (sock->connect(port, address.c_str()) < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", address.c_str(), port);
        sock->close();
        exit(EXIT_FAILURE);
    }
    printf("Connected to %s:%d\n", address.c_str(), port);

    // send single file
    if (send_file(filename, *sock, 0) < 0)
        fprintf(stderr, "Failed to send file %s\n", filename);
    else
        fprintf(stderr, "Sent file %s\n", filename);
    
    send_command(*sock, 'd');

    sock->close();
    fprintf(stderr, "Closed connection to %s:%d\n", address.c_str(), port);
}

int send_directory(std::string dir_path, woo200::ClientSocket &sock, int deepness=1)
{
    struct dirent *entry = nullptr;
    DIR *dp = nullptr;

    dp = opendir(dir_path.c_str());
    if (dp != nullptr) {
        while ((entry = readdir(dp)))
        {
            if (entry->d_type == DT_DIR) {
                std::string dir_name = entry->d_name;
                if (dir_name == "." || dir_name == "..")
                    continue;
                std::string new_dir_path = dir_path + "/" + dir_name;
                send_directory(new_dir_path, sock, deepness+1);
            }
            else {
                std::string file_name = entry->d_name;
                std::string file_path = dir_path + "/" + file_name;
                std::string name = base_name(file_path, deepness);
                printf("Sending file %s (name: %s)\n", file_path.c_str(), name.c_str());

                if (send_file(file_path, sock, deepness) < 0)
                    fprintf(stderr, "Failed to send file %s\n", file_path.c_str());
                // if (sock.recv((char*) &cmd, sizeof(cmd)) < 0) {
                //     fprintf(stderr, "Connection closed abruptly!");
                //     exit(EXIT_FAILURE);
                // }
                // if (cmd.command != 'c')
                //     fprintf(stderr, "Failed to receive complete command\n");
            }
        }
    }

    closedir(dp);
}

void send_recursive(std::string address, int port, const char* dir_path)
{
    woo200::ClientSocket* sock = new woo200::ClientSocket();
    if (sock->connect(port, address.c_str()) < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", address.c_str(), port);
        sock->close();
        exit(EXIT_FAILURE);
    }
    printf("Connected to %s:%d\n", address.c_str(), port);

    // send directory
    send_directory(dir_path, *sock);

    // Tell server we're done
    send_command(*sock, 'd');

    sock->close();
}

// ################################
// ################################
// ############# MAIN #############
// ################################
// ################################


// Print usage and exit
void exit_usage(const char* program_name)
{
    fprintf(stderr, "Usage: %s [-spa] [file...]\n", program_name);
    fprintf(stderr, "    -s : Run as server\n");
    fprintf(stderr, "    -p : Port number\n");
    fprintf(stderr, "    -a : Address\n");
    fprintf(stderr, "    -r : Recursively send directory\n");
    exit(EXIT_FAILURE);
}

/*
Command usage:
    ./file_transfer [-r ip:port] [file...]
    -r : Host file server
*/
int main(int argc, char** argv)
{
    int opt;
    enum { SERVER_MODE, SEND_MODE, SEND_RECURSIVE_MODE } mode = SEND_MODE;

    int port = 59661;
    std::string address = "0.0.0.0";

    while ((opt = getopt(argc, argv, "rsp:a:")) != -1) {
        switch (opt) {
        case 's': mode = SERVER_MODE; break;
        case 'r': mode = SEND_RECURSIVE_MODE; break;
        case 'p':
            port = atoi(optarg);
            if (port <= 0) {
                fprintf(stderr, "Invalid port number: %s\n", optarg);
                exit_usage(argv[0]);
            }
            break;
        case 'a':
            address = optarg;
            break;
        default:
            exit_usage(argv[0]);
        }
    }

    switch (mode) {
        case SERVER_MODE:
            start_receive(address, port);
            break;
        case SEND_MODE:
            if (address == "0.0.0.0")
                address = "127.0.0.1";
            if (optind >= argc)
                exit_usage(argv[0]);
            send_file(address, port, argv[optind]);
            break;
        case SEND_RECURSIVE_MODE:
            if (address == "0.0.0.0")
                    address = "127.0.0.1";
            if (optind >= argc)
                exit_usage(argv[0]);
            send_recursive(address, port, argv[optind]);
            break;
    };

    return 0;
}