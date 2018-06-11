//
// Created by pair on 07/06/18.
//

#include <string>
#include <zmq.h>
#include <cassert>
#include "SocketImpl.h"
#include "ZeroMQWrapperException.h"

using namespace Common::ZeroMQWrapperImpl;

Common::ZeroMQWrapperImpl::SocketImpl::SocketImpl(Common::ZeroMQWrapperImpl::ContextHolder &context, int type)
        : m_socket(context,type)
{
}

void Common::ZeroMQWrapperImpl::SocketImpl::setTimeout(int timeoutMs)
{
    int rc = zmq_setsockopt(m_socket.skt(),ZMQ_RCVTIMEO,&timeoutMs, sizeof(timeoutMs));
    if (rc != 0)
    {
        throw ZeroMQWrapperException("Failed to set timeout for receiving");
    }
    rc = zmq_setsockopt(m_socket.skt(),ZMQ_SNDTIMEO,&timeoutMs, sizeof(timeoutMs));
    if (rc != 0)
    {
        throw ZeroMQWrapperException("Failed to set timeout for sending");
    }
}

void Common::ZeroMQWrapperImpl::SocketImpl::connect(const std::string &address)
{
    int rc = zmq_connect(m_socket.skt(), address.c_str());
    if (rc != 0)
    {
        throw ZeroMQWrapperException(std::string("Failed to connect to ")+address);
    }
}

void Common::ZeroMQWrapperImpl::SocketImpl::listen(const std::string &address)
{
    int rc = zmq_bind(m_socket.skt(), address.c_str());
    if (rc != 0)
    {
        throw ZeroMQWrapperException(std::string("Failed to bind to ")+address);
    }
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
