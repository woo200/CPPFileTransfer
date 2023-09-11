#include "main.hpp"

#define CHUNK_SIZE 128000

// ################################
// ################################
// ############ SERVER ############
// ################################
// ################################

void receive_file(woo200::ClientSocket client)
{
    woo200::FileHeader header(&client);
    const char* filename = header.get_filename();

    unsigned long size = header.get_size();
    printf("Receiving file %s (%lu bytes)\n", filename, size);

    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char buf[CHUNK_SIZE];
    int read_len;
    while (size > 0 && (read_len = client.recv(buf, CHUNK_SIZE)) > 0) {
        fwrite(buf, 1, read_len, fp);
        size -= read_len;
    }

    fclose(fp);
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
    woo200::ClientSocket client = sock.accept();
    std::cout << "Accepted connection from " << inet_ntoa(client.get_addr().sin_addr) << "\n";

    woo200::CommandPacket cmd;
    do {
        if (client.recv((char*) &cmd, sizeof(cmd)) < 0) {
            std::cout << "Connection Closed" << std::endl;
            break;
        }
        switch (cmd.command) {
            case 'f':
                receive_file(client);
                break;
            case 'd':
                std::cout << "Done\n";
                break;
            default:
                fprintf(stderr, "Unknown command: %c\n", cmd.command);
                break;
        }
    } while (cmd.command != 'd');
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
// ################################
// ################################
// ############ CLIENT ############
// ################################
// ################################
// ################################

int send_file(std::string file_path, woo200::ClientSocket sock, int deepness)
{
    FILE* fp = fopen(file_path.c_str(), "r");
    if (fp == NULL) 
        return -1;
    
    // Get file length
    fseek(fp, 0, SEEK_END);
    unsigned long file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Tell server it's receiving a file
    woo200::CommandPacket cmd = { 'f' };
    sock.send((char*) &cmd, sizeof(cmd));

    // Send file header packet
    std::string basename = base_name(file_path, deepness);
    woo200::FileHeader header(basename.c_str(), file_len);
    woo200::PrefixedLengthByteArray data = header.to_byte_array();
    sock.send(data.data, data.length);

    // Send file data
    char buf[CHUNK_SIZE];
    int read_len;
    while ((read_len = fread(buf, 1, CHUNK_SIZE, fp)) > 0) {
        sock.send(buf, read_len);
    }
    fclose(fp);

    return 0;
}

void send_file(std::string address, int port, const char* filename)
{
    woo200::ClientSocket sock;
    if (sock.connect(port, address.c_str()) < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", address.c_str(), port);
        sock.close();
        exit(EXIT_FAILURE);
    }
    printf("Connected to %s:%d\n", address.c_str(), port);

    // send single file
    if (send_file(filename, sock, 0) < 0)
        fprintf(stderr, "Failed to send file %s\n", filename);
    else
        fprintf(stderr, "Sent file %s\n", filename);
    
    // Tell server we're done
    woo200::CommandPacket cmd = { 'd' };
    sock.send((char*) &cmd, sizeof(cmd));

    sock.close();
    fprintf(stderr, "Closed connection to %s:%d\n", address.c_str(), port);
}

int send_directory(std::string dir_path, woo200::ClientSocket sock, int deepness=1)
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
                
                // Tell server it's receiving a file
                woo200::CommandPacket cmd = { 'f' };
                sock.send((char*) &cmd, sizeof(cmd));

                if (send_file(file_path, sock, deepness) < 0)
                    fprintf(stderr, "Failed to send file %s\n", file_path.c_str());
            }
        }
    }

    closedir(dp);
}

void send_recursive(std::string address, int port, const char* dir_path)
{
    woo200::ClientSocket sock;
    if (sock.connect(port, address.c_str()) < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", address.c_str(), port);
        sock.close();
        exit(EXIT_FAILURE);
    }
    printf("Connected to %s:%d\n", address.c_str(), port);

    // send directory
    send_directory(dir_path, sock);

    // Tell server we're done
    woo200::CommandPacket cmd = { 'd' };
    sock.send((char*) &cmd, sizeof(cmd));

    sock.close();
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