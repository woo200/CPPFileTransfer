#include "filedef.hpp"

namespace woo200
{
    PFileHeader::PFileHeader(std::string filename, unsigned long size)
    {
        this->filename = new PString(filename);
        this->size = new PUlong(size);

        this->add_field(this->filename);
        this->add_field(this->size);
    }
    PFileHeader::~PFileHeader()
    {
        delete this->filename;
        delete this->size;
    }
    
    std::string PFileHeader::get_filename()
    {
        return this->filename->get_value();
    }
    unsigned long PFileHeader::get_filesize()
    {
        return this->size->get_value();
    }
    void PFileHeader::set_filename(std::string filename)
    {
        this->filename->set_value(filename);
    }
    void PFileHeader::set_filesize(unsigned long size)
    {
        this->size->set_value(size);
    }

    PCommand::PCommand(char command)
    {
        this->command = command;
    }
    
    char PCommand::get_command()
    {
        return this->command;
    }
    void PCommand::set_command(char command)
    {
        this->command = command;
    }
    std::string PCommand::get_i_data()
    {
        return std::string(&this->command, 1);
    }
    int PCommand::read_i_data(ClientSocket &socket)
    {
        return socket.recv(&this->command, 1);
    }
} 
