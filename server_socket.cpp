#include "server_socket.hpp"

namespace woo200
{
    ServerSocket::ServerSocket()
    {
        this->sock = socket(AF_INET, SOCK_STREAM, 0);
        this->bind_addr.sin_family = AF_INET;
    }
    ServerSocket::~ServerSocket()
    {
        this->close();
    }

    void ServerSocket::close()
    {
        ::close(this->sock);
    }
    int ServerSocket::bind(const char* address, int port)
    {
        this->bind_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, address, &this->bind_addr.sin_addr) <= 0)
        {
            return -1;
        }
        return ::bind(this->sock, (struct sockaddr*) &this->bind_addr, sizeof(this->bind_addr));
    }
    int ServerSocket::bind(sockaddr_in* addr)
    {
        this->bind_addr = *addr;
        ::setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &addr, sizeof(addr));
        return ::bind(this->sock, (struct sockaddr*) &this->bind_addr, sizeof(this->bind_addr));
    }
    int ServerSocket::listen(int backlog)
    {
        return ::listen(this->sock, backlog);
    }
    ClientSocket ServerSocket::accept()
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = ::accept(this->sock, (struct sockaddr*) &client_addr, &client_addr_len);
        ClientSocket client(client_sock, client_addr);

        return client;
    }
}