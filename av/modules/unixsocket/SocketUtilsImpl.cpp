// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "SocketUtilsImpl.h"

#include <stdexcept>

std::deque<uint8_t> unixsocket::splitInto7Bits(unsigned length)
{
    std::deque<uint8_t> bytes;
    while (length > 0)
    {
        uint8_t c = length & static_cast<uint8_t>(127);
        bytes.push_front(c);
        length = length >> static_cast<uint8_t>(7);
    }
    return bytes;
}

std::unique_ptr<uint8_t[]> unixsocket::addTopBitAndPutInBuffer(const std::deque<uint8_t>& bytes)
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

unixsocket::buffer_ptr_t unixsocket::getLength(size_t length, size_t& bufferSize)
{
    if (length == 0)
    {
        throw std::runtime_error("Attempting to write length of zero");
    }

    auto bytes = splitInto7Bits(length);
    bufferSize = bytes.size();
    return addTopBitAndPutInBuffer(bytes);
}
