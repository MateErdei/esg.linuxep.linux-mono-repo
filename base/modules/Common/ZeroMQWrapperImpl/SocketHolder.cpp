/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketHolder.h"
#include "ZeroMQWrapperException.h"

#include <zmq.h>
#include <cassert>

Common::ZeroMQWrapperImpl::SocketHolder::SocketHolder(void *zmq_socket)
    : m_socket(zmq_socket)
{
}

Common::ZeroMQWrapperImpl::SocketHolder::~SocketHolder()
{
    if (m_socket != nullptr)
    {
        zmq_close(m_socket);
    }
}

void *Common::ZeroMQWrapperImpl::SocketHolder::skt()
{
    return m_socket;
}

void Common::ZeroMQWrapperImpl::SocketHolder::reset(void *zmq_socket)
{
    if (m_socket != nullptr)
    {
        zmq_close(m_socket);
    }
    m_socket = zmq_socket;
}

Common::ZeroMQWrapperImpl::SocketHolder::SocketHolder(Common::ZeroMQWrapperImpl::ContextHolder &context, const int type)
    : m_socket(nullptr)
{
    assert(context.ctx() != nullptr);
    m_socket = zmq_socket(context.ctx(), type);
    if (m_socket == nullptr)
    {
        throw ZeroMQWrapperException("Failed to create socket");
    }
}
