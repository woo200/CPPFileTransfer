#pragma once

#include <string.h>  
#include "libs/packets/base_packet.hpp"

namespace woo200 
{   
    class PFileHeader : public Packet
    {
        const char* filename;
        unsigned long size;
    public:
        private:
            PString* filename;
            PUlong* size;
        public:
            PFileHeader(std::string filename = "", unsigned long size = 0);
            ~PFileHeader();

            std::string get_filename();
            unsigned long get_filesize();
            void set_filename(std::string filename);
            void set_filesize(unsigned long size);
    };

    class PCommand : public Packet
    {
        private:
            char command;

            void read_i_data(ClientSocket &socket);
            std::string get_i_data();
        public:
            PCommand(char command = 0);

            char get_command();
            void set_command(char command);
    };
}