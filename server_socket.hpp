#pragma once

#include "client_socket.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

namespace woo200 {
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