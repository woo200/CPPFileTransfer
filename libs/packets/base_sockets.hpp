#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

namespace woo200 {
    class ClientSocket {
        struct sockaddr_in remote_addr;
        int sock;
        bool connected;
    private:
        int connect();
    public:
        ClientSocket();
        ClientSocket(int sock, sockaddr_in addr);
        ~ClientSocket();

        sockaddr_in get_addr();

        int connect(int port, const char* ip);
        int connect(int port, sockaddr_in* addr);
        int send(void* msg, int len);
        int recv(void* buf, int len, int flags=0);
        void close();
    };
    
    class ServerSocket {
        struct sockaddr_in bind_addr;
        int sock;
    public:
        ServerSocket();
        ~ServerSocket();

        int bind(const char* address, int port);
        int bind(sockaddr_in* addr);
        int listen(int backlog);
        ClientSocket* accept();

        void close();
    };
}