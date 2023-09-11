#pragma once

#include <string.h>  
#include "client_socket.hpp"

namespace woo200 
{
    typedef struct PrefixedLengthByteArray
    {
        unsigned int length;
        char* data;
    } PrefixedLengthByteArray;

    typedef struct CommandPacket
    {
        char command;
    } CommandPacket;
    
    class FileHeader
    {
        const char* filename;
        unsigned long size;
    public:
        FileHeader(const char* filename, unsigned long size);
        FileHeader(woo200::ClientSocket* sock);
        PrefixedLengthByteArray to_byte_array();

        const char* get_filename();
        unsigned long get_size();
    };
}