/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AutoFd.h"

#include <unistd.h>

using namespace datatypes;

void AutoFd::reset(int fd)
{
    close();
    m_fd = fd;
}

void AutoFd::close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
    }
    m_fd = -1;
}

AutoFd::AutoFd(int fd)
        : m_fd(fd)
{
}

AutoFd::AutoFd(AutoFd&& other) noexcept
    : m_fd(other.m_fd)
{
    other.m_fd = -1;
}

AutoFd::~AutoFd()
{
    close();
}

int AutoFd::release()
{
    int fd = m_fd;
    m_fd = -1;
    return fd;
}
