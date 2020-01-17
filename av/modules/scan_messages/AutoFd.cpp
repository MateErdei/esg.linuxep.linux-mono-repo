/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AutoFd.h"

#include <unistd.h>

void scan_messages::AutoFd::reset(int fd)
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
    }
    m_fd = fd;
}

scan_messages::AutoFd::AutoFd(int fd)
        : m_fd(fd)
{
}

scan_messages::AutoFd::AutoFd(scan_messages::AutoFd&& other) noexcept
    : m_fd(other.m_fd)
{
    other.m_fd = -1;
}

scan_messages::AutoFd& scan_messages::AutoFd::operator=(scan_messages::AutoFd&& other) noexcept
{
    int tempfd = other.m_fd;
    other.m_fd = m_fd;
    m_fd = tempfd;
    return *this;
}

scan_messages::AutoFd::~AutoFd()
{
    reset(-1);
}

int scan_messages::AutoFd::release()
{
    int fd = m_fd;
    m_fd = -1;
    return fd;
}
