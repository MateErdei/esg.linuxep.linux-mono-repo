/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketUtils.h"

#include "datatypes/Print.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <memory>

#include <unistd.h>

static std::deque<uint8_t> splitInto7Bits(unsigned length)
{
    std::deque<uint8_t> bytes;
    while (length > 0)
    {
        uint8_t c = length & static_cast<uint8_t>(127);
        bytes.push_front(c);
        length >>= static_cast<uint8_t>(7);
    }
    return bytes;
}

static std::unique_ptr<uint8_t[]> addTopBitAndPutInBuffer(const std::deque<uint8_t>& bytes)
{
    const uint8_t TOP_BIT = 0x80;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bytes.size()]);
    for(size_t i=0; i < bytes.size()-1; i++)
    {
        buffer[i] = bytes[i] | TOP_BIT;
    }
    buffer[bytes.size()-1] = bytes[bytes.size()-1];
    return buffer;
}

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
        else
        {
            perror("Reading length");
            return -1;
        }
    }
}
