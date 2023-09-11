#include "client_socket.hpp"

namespace woo200
{   
    ClientSocket::ClientSocket()
    {
        this->sock = socket(AF_INET, SOCK_STREAM, 0);
        this->remote_addr.sin_family = AF_INET;
        this->connected = false;
    }
    ClientSocket::ClientSocket(int sock, sockaddr_in addr)
    {
        this->sock = sock;
        this->remote_addr = addr;

        this->connected = true;
    }
    ClientSocket::~ClientSocket()
    {
        this->close();
    }

    sockaddr_in ClientSocket::get_addr()
    {
        return this->remote_addr;
    }
    int ClientSocket::connect()
    {
        if (this->connected)
            return 0;

        int res = ::connect(this->sock, (struct sockaddr*) &this->remote_addr, sizeof(this->remote_addr));
        if (res >= 0)
            this->connected = true;

        return res;
    }
    int ClientSocket::connect(int port, const char* ip)
    {
        if (this->connected)
            return 0;

        this->remote_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &this->remote_addr.sin_addr) <= 0)
            return -1;

        return this->connect();
    }
    int ClientSocket::connect(int port, sockaddr_in* addr)
    {
        this->remote_addr = *addr;
        return this->connect();
    }
    int ClientSocket::send(const char* msg, int len)
    {
        if (!this->connected)
            return -1;

        return ::send(this->sock, msg, len, 0);
    }
    int ClientSocket::recv(char* buf, int len, int flags)
    {
        if (!this->connected)
            return -1;

        return ::recv(this->sock, buf, len, flags);
    }
    void ClientSocket::close()
    {
        ::close(this->sock);
    }
}
