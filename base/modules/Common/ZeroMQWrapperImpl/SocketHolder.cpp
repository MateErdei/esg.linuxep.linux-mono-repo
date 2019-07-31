/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketHolder.h"

#include "ZeroMQWrapperException.h"

#include <cassert>
#include <zmq.h>

Common::ZeroMQWrapperImpl::SocketHolder::SocketHolder(void* zmq_socket) : m_socket(zmq_socket) {}

Common::ZeroMQWrapperImpl::SocketHolder::~SocketHolder()
{
    if (m_socket != nullptr)
    {
        zmq_close(m_socket);
    }
}

void* Common::ZeroMQWrapperImpl::SocketHolder::skt()
{
    return m_socket;
}

void Common::ZeroMQWrapperImpl::SocketHolder::reset(void* zmq_socket)
{
    if (m_socket != nullptr)
    {
        zmq_close(m_socket);
    }
    m_socket = zmq_socket;
}

Common::ZeroMQWrapperImpl::SocketHolder::SocketHolder(
    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr& context,
    const int type) :
    m_socket(nullptr)
{
    reset(context, type);
}

void Common::ZeroMQWrapperImpl::SocketHolder::reset(
    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr& context,
    int type)
{
    assert(context.get() != nullptr);
    assert(context->ctx() != nullptr);
    if (m_socket != nullptr)
    {
        zmq_close(m_socket);
    }
    m_socket = zmq_socket(context->ctx(), type);
    if (m_socket == nullptr)
    {
        throw ZeroMQWrapperException("Failed to create socket");
    }
    constexpr int64_t maxSize = 10 * 1024 * 1024; // 10 MB
    if (-1 == zmq_setsockopt(m_socket, ZMQ_MAXMSGSIZE, &maxSize, sizeof(maxSize)))
    {
        throw std::logic_error("Failed to configure option to limit max size of message on the socket");
    }
}

void* Common::ZeroMQWrapperImpl::SocketHolder::release()
{
    void* skt = m_socket;
    m_socket = nullptr;
    return skt;
}
