// Copyright 2020-2022 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <deque>
#include <memory>

namespace unixsocket
{
    std::deque<uint8_t> splitInto7Bits(unsigned length);
    using buffer_ptr_t = std::unique_ptr<uint8_t[]>;
    buffer_ptr_t addTopBitAndPutInBuffer(const std::deque<uint8_t>& bytes);
    buffer_ptr_t getLength(size_t length, size_t& bufferSize);

}
