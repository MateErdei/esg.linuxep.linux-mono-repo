/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketImpl.h"

#include "SocketUtil.h"
#include "ZeroMQWrapperException.h"

#include <cassert>
#include <string>
#include <zmq.h>

using namespace Common::ZeroMQWrapperImpl;

Common::ZeroMQWrapperImpl::SocketImpl::SocketImpl(Common::ZeroMQWrapperImpl::ContextHolderSharedPtr context, int type) :
    m_context(context),
    m_socket(context, type)
{
    m_appliedSettings.socketType = type;
}

void Common::ZeroMQWrapperImpl::SocketImpl::setTimeout(int timeoutMs)
{
    m_appliedSettings.timeout = timeoutMs;
    SocketUtil::setTimeout(m_socket, timeoutMs);
}

void Common::ZeroMQWrapperImpl::SocketImpl::setConnectionTimeout(int timeoutMs)
{
    m_appliedSettings.connectionTimeout = timeoutMs;
    SocketUtil::setConnectionTimeout(m_socket, timeoutMs);
}

void Common::ZeroMQWrapperImpl::SocketImpl::connect(const std::string& address)
{
    m_appliedSettings.address = address;
    m_appliedSettings.connectionSetup = AppliedSettings::ConnectionSetup::Connect;
    SocketUtil::connect(m_socket, address);
}

void Common::ZeroMQWrapperImpl::SocketImpl::listen(const std::string& address)
{
    m_appliedSettings.address = address;
    m_appliedSettings.connectionSetup = AppliedSettings::ConnectionSetup::Listen;
    SocketUtil::listen(m_socket, address);
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

void SocketImpl::refresh()
{
    assert(m_context.get() != nullptr);
    assert(m_context->ctx() != nullptr);
    m_socket.reset(m_context, m_appliedSettings.socketType);
    if (m_appliedSettings.timeout != -1)
    {
        setTimeout(m_appliedSettings.timeout);
    }
    if (m_appliedSettings.connectionTimeout != -1)
    {
        setConnectionTimeout(m_appliedSettings.connectionTimeout);
    }
    switch (m_appliedSettings.connectionSetup)
    {
        case AppliedSettings::ConnectionSetup::Listen:
            listen(m_appliedSettings.address);
            break;
        case AppliedSettings::ConnectionSetup::Connect:
            connect(m_appliedSettings.address);
            break;
        default:
            assert(false); // should never occur.
            break;
    }
}

int SocketImpl::timeout() const
{
    return m_appliedSettings.timeout;
}
