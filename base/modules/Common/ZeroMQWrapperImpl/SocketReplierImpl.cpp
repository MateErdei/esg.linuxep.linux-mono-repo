//
// Created by pair on 07/06/18.
//
#include "SocketReplierImpl.h"
#include "SocketUtil.h"

#include <zmq.h>

Common::ZeroMQWrapperImpl::SocketReplierImpl::SocketReplierImpl(Common::ZeroMQWrapperImpl::ContextHolder &context)
        : SocketImpl(context, ZMQ_REP)
{

}

std::vector<std::string> Common::ZeroMQWrapperImpl::SocketReplierImpl::read()
{
    return SocketUtil::read(m_socket);
}

void Common::ZeroMQWrapperImpl::SocketReplierImpl::write(const std::vector<std::string> &data)
{
    SocketUtil::write(m_socket,data);
}
