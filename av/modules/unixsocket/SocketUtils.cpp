/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtils.h"

#include "datatypes/Print.h"

#include <cassert>
#include <cstdint>

#include <unistd.h>
#include <cstdio>

void unixsocket::writeLength(int socketfd, int length)
{
    ssize_t bytes_written;
    // TODO implement proper length writing
    if (length == 0)
    {
        throw std::runtime_error("Attempting to write length of zero");
    }
    if (length < 128)
    {
        uint8_t c = length;
        bytes_written = ::write(socketfd, &c, 1);
        static_cast<void>(bytes_written);
    }
    else
    {
        // divide into 7-bit chunks, and send big-endian with top bit set.
        assert(false);
        throw std::runtime_error("Length > 128 not supported yet!");
    }
}

bool unixsocket::writeLengthAndBuffer(int socket_fd, const std::string& buffer)
{
    writeLength(socket_fd, buffer.size());

    ssize_t bytes_written = ::write(socket_fd, buffer.c_str(), buffer.size());
    if (static_cast<unsigned>(bytes_written) != buffer.size())
    {
        PRINT("Failed to write buffer to unix socket");
        return false;
    }
    return true;
}

int unixsocket::readLength(int socket_fd)
{
    int32_t total = 0;
    uint8_t byte; // For some reason clang-tidy thinks this is signed
    const uint8_t TOP_BIT = 0x80;
    while (true)
    {
        int count = read(socket_fd, &byte, 1);
        if (count == 1)
        {
            if ((byte & TOP_BIT) == 0)
            {
                total = total * 128 + byte;
                return total;
            }
            total = total * 128 + (byte ^ TOP_BIT);
            if (total > 128 * 1024)
            {
                return -1;
            }
        }
        else
        {
            perror("Reading length");
            return -1;
        }
    }
}
