/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>
#include <zmq.h>
#include "SocketImpl.h"
#include "ZeroMQWrapperException.h"
#include "SocketUtil.h"

using namespace Common::ZeroMQWrapperImpl;

Common::ZeroMQWrapperImpl::SocketImpl::SocketImpl(Common::ZeroMQWrapperImpl::ContextHolder &context, int type)
        : m_socket(context,type)
{
}

void Common::ZeroMQWrapperImpl::SocketImpl::setTimeout(int timeoutMs)
{
    SocketUtil::setTimeout(m_socket,timeoutMs);
}

void Common::ZeroMQWrapperImpl::SocketImpl::setConnectionTimeout(int timeoutMs)
{
    SocketUtil::setConnectionTimeout(m_socket,timeoutMs);
}

void Common::ZeroMQWrapperImpl::SocketImpl::connect(const std::string &address)
{
    SocketUtil::connect(m_socket, address);
}

void Common::ZeroMQWrapperImpl::SocketImpl::listen(const std::string &address)
{
    SocketUtil::listen(m_socket,address);
}

int SocketImpl::fd()
{
    int fd;
    size_t length = sizeof(fd);
    int rc = zmq_getsockopt(m_socket.skt(), ZMQ_FD, &fd, &length);
    if (rc == 0)
    {
        return fd;
    }
    return -1;
}
