// Copyright 2023 Sophos All rights reserved.
#include "ReadBufferAsync.h"

#include "capnp/common.h"

#include <cassert>

unixsocket::ReadBufferAsync::ReadBufferAsync(
    unixsocket::ReadBufferAsync::ISystemCallWrapperSharedPtr systemCalls,
    size_t defaultSize) :
    systemCalls_(std::move(systemCalls))
{
    buffer_size_ = 1 + defaultSize / sizeof(capnp::word);
    proto_buffer_ = kj::heapArray<capnp::word>(buffer_size_);
}

bool unixsocket::ReadBufferAsync::setLength(size_t size)
{
    if (static_cast<uint32_t>(size) > (buffer_size_ * sizeof(capnp::word)))
    {
        buffer_size_ = 1 + size / sizeof(capnp::word);
        proto_buffer_ = kj::heapArray<capnp::word>(buffer_size_);
        return true;
    }
    expectedSize_ = size;
    position_ = 0;
    return false;
}

int unixsocket::ReadBufferAsync::read(int fd)
{
    const auto start = reinterpret_cast<char*>(proto_buffer_.begin()) + position_;
    const auto amount = expectedSize_ - position_;
    const auto bytes_read = systemCalls_->read(fd, start, amount);
    if (bytes_read > 0)
    {
        position_ += bytes_read;
    }
    assert(position_ <= expectedSize_);
    return bytes_read;
}
