//
// Created by pair on 07/06/18.
//

#include <string>
#include <zmq.h>
#include <cassert>
#include "SocketImpl.h"

using namespace Common::ZeroMQWrapperImpl;

Common::ZeroMQWrapperImpl::SocketImpl::SocketImpl(Common::ZeroMQWrapperImpl::ContextHolder &context, int type)
        : m_socket(context,type)
{
}

void Common::ZeroMQWrapperImpl::SocketImpl::setTimeout(int timeoutMs)
{
    int rc = zmq_setsockopt(m_socket.skt(),ZMQ_RCVTIMEO,&timeoutMs, sizeof(timeoutMs));
    assert(rc == 0);
    rc = zmq_setsockopt(m_socket.skt(),ZMQ_SNDTIMEO,&timeoutMs, sizeof(timeoutMs));
    assert(rc == 0);
}

void Common::ZeroMQWrapperImpl::SocketImpl::connect(const std::string &address)
{
    int rc = zmq_connect(m_socket.skt(), address.c_str());
    assert(rc == 0);
}

void Common::ZeroMQWrapperImpl::SocketImpl::listen(const std::string &address)
{
    int rc = zmq_bind(m_socket.skt(), address.c_str());
    assert(rc == 0);
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

void SocketImpl::setNonBlocking()
{
}

