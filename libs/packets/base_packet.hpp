#pragma once

#include "base_sockets.hpp"
#include <vector>

namespace woo200
{
    // BASE CLASS (Not Implimented)
    class Packet
    {
        private:
            virtual std::string get_i_data();
            virtual void read_i_data(ClientSocket &socket);

            std::vector<Packet*> packets;
            std::vector<Packet*> fields;
            int size = 0;
            unsigned long index = 0;
        protected:
            void add_field(Packet* packet);
        public:
            Packet();

            virtual void read_from_socket(ClientSocket &socket);
            virtual std::string get_data();
            virtual int get_size();
            virtual int send_to_socket(ClientSocket &socket);

            Packet* next();
            Packet operator/(Packet &packet);
    };
    
    class PInt : public Packet
    {
        private:
            std::string get_i_data();
            void read_i_data(ClientSocket &socket);
            int value;
        public:
            PInt(int value = 0);
            int get_value();
            void set_value(int value);
    };

    class PString : public Packet
    {
        private:
            std::string get_i_data();
            void read_i_data(ClientSocket &socket);
            std::string data;
        public:
            PString(std::string data = "");
            std::string get_value();
            void set_value(std::string data);
    };

    class PUlong : public Packet
    {
        private:
            std::string get_i_data();
            void read_i_data(ClientSocket &socket);
            unsigned long value;
        public:
            PUlong(unsigned long value = 0);
            unsigned long get_value();
            void set_value(unsigned long value);
    };
}
