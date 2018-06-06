//
// Created by pair on 06/06/18.
//

#include "SocketHolder.h"

#include <zmq.h>

Common::ZeroMQWrapperImpl::SocketHolder::SocketHolder(void *zmq_socket)
{
    m_socket = zmq_socket;
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
