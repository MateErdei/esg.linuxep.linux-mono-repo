// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "AutoFd.h"

#include <utility>

#include <unistd.h>

using namespace datatypes;

void AutoFd::reset(int fd)
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
    }
    m_fd = fd;
}

void AutoFd::close()
{
    reset(invalid_fd);
}


AutoFd::AutoFd(int fd) noexcept
        : m_fd(fd)
{
}

AutoFd::AutoFd(AutoFd&& other) noexcept
    : m_fd(std::exchange(other.m_fd, invalid_fd))
{
}

AutoFd& AutoFd::operator=(AutoFd&& other) noexcept
{
    std::swap(m_fd, other.m_fd);
    return *this;
}

AutoFd::~AutoFd()
{
    reset();
}

int AutoFd::release()
{
    return std::exchange(m_fd, invalid_fd);
}

bool AutoFd::operator==(const AutoFd& other) const
{
    return m_fd == other.m_fd;
}
