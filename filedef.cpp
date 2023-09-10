#include "filedef.hpp"

namespace woo200
{
    FileHeader::FileHeader(const char* filename, unsigned long size)
    {
        this->filename = filename;
        this->size = size;
    }
    FileHeader::FileHeader(woo200::ClientSocket* sock)
    {
        unsigned short filename_len;
        sock->recv((char*) &filename_len, sizeof(unsigned short));
        char* filename = new char[filename_len];
        sock->recv(filename, filename_len);
        unsigned long size;
        sock->recv((char*) &size, sizeof(unsigned long));

        char* filename_with_null = new char[filename_len + 1];
        memcpy(filename_with_null, filename, filename_len);
        filename_with_null[filename_len] = '\0';

        this->filename = filename_with_null;
        this->size = size;
    }
    PrefixedLengthByteArray FileHeader::to_byte_array()
    {
        unsigned short filename_len = strlen(this->filename);
        // filename_len, filename, size
        unsigned int total_len = sizeof(short) + filename_len + sizeof(unsigned long);

        char* data = new char[total_len];
        memcpy(data, &filename_len, sizeof(unsigned short));
        memcpy(data + sizeof(unsigned short), this->filename, filename_len);
        memcpy(data + sizeof(unsigned short) + filename_len, &this->size, sizeof(unsigned long));
        
        PrefixedLengthByteArray ret = {total_len, data};
        return ret;
    }
    const char* FileHeader::get_filename()
    {
        return this->filename;
    }
    unsigned long FileHeader::get_size()
    {
        return this->size;
    }
} 
