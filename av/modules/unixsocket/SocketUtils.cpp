/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtils.h"
#include "SocketUtilsImpl.h"

#include "datatypes/Print.h"

#include <cstdint>
#include <cstdio>
#include <memory>

#include <unistd.h>

void unixsocket::writeLength(int socketfd, unsigned length)
{
    if (length == 0)
    {
        throw std::runtime_error("Attempting to write length of zero");
    }

    auto bytes = splitInto7Bits(length);
    auto buffer = addTopBitAndPutInBuffer(bytes);
    ssize_t bytes_written;
    bytes_written = ::write(socketfd, buffer.get(), bytes.size());
    if (bytes_written != static_cast<ssize_t>(bytes.size()))
    {
        throw std::runtime_error("Failed to write length to socket");
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
        else if (count == -1)
        {
            perror("Reading FD returned error");
            return -1;
        }
        else if (count == 0)
        {
            perror("Reading FD returned EOF");
            return 0;
        }
    }
}
