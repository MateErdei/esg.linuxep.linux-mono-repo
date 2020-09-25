/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtils.h"
#include "SocketUtilsImpl.h"

#include "Logger.h"

#include <cstdint>
#include <cstring>
#include <memory>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

void unixsocket::writeLength(int socket_fd, unsigned length)
{
    if (length == 0)
    {
        throw std::runtime_error("Attempting to write length of zero");
    }

    auto bytes = splitInto7Bits(length);
    auto buffer = addTopBitAndPutInBuffer(bytes);
    ssize_t bytes_written;
    bytes_written = ::send(socket_fd, buffer.get(), bytes.size(), MSG_NOSIGNAL);
    if (bytes_written != static_cast<ssize_t>(bytes.size()))
    {
        throw environmentInterruption();
    }
}

bool unixsocket::writeLengthAndBuffer(int socket_fd, const std::string& buffer)
{
    writeLength(socket_fd, buffer.size());

    ssize_t bytes_written = ::send(socket_fd, buffer.c_str(), buffer.size(), MSG_NOSIGNAL);
    if (static_cast<unsigned>(bytes_written) != buffer.size())
    {
        LOGWARN("Failed to write buffer to unix socket");
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
        ssize_t count = read(socket_fd, &byte, 1);
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
            LOGERROR("Reading socket returned error: " << std::strerror(errno));
            return -1;
        }
        else if (count == 0)
        {
            // can't call perror since 0 return doesn't set errno
            // should only return count == 0 on EOF, so we don't need to continue the connection
            return -2;
        }
    }
}
