// Copyright 2023 Sophos All rights reserved.

#include "ReadLengthAsync.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

unixsocket::ReadLengthAsync::ReadLengthAsync(
        unixsocket::ReadLengthAsync::ISystemCallWrapperSharedPtr systemCalls,
        ssize_t maxSize)
    : systemCalls_(std::move(systemCalls))
      , maxSize_(maxSize)
{
}

ssize_t unixsocket::ReadLengthAsync::getLength()
{
    assert(complete_);
    return currentLength_;
}

int unixsocket::ReadLengthAsync::read(int fd)
{
    assert(!complete_);
    uint8_t byte = 0; // For some reason clang-tidy thinks this is signed
    const uint8_t TOP_BIT = 0x80;
    int read = 0;
    while (true)
    {
        ssize_t count = systemCalls_->read(fd, &byte, 1);
        if (count == 1)
        {
            read++;
            if (byte == 0x80 && currentLength_ == 0)
            {
                errno = EINVAL;
                return -1;
            }
            else if ((byte & TOP_BIT) == 0)
            {
                currentLength_ = currentLength_ * 128 + byte;
                complete_ = true;
                return read;
            }
            currentLength_ = currentLength_ * 128 + (byte ^ TOP_BIT);
            if (currentLength_ > maxSize_)
            {
                errno = E2BIG;
                return -1;
            }
        }
        else if (count == -1)
        {
            LOGDEBUG("Reading socket returned error: " << common::safer_strerror(errno));
            return -1;
        }
        else if (count == 0)
        {
            // Async - go back to waiting
            return read;
        }
    }
}

bool unixsocket::ReadLengthAsync::complete()
{
    return complete_;
}

void unixsocket::ReadLengthAsync::reset()
{
    complete_ = false;
    currentLength_ = 0;
}
