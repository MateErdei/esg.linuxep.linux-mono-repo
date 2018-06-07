//
// Created by pair on 07/06/18.
//

#include "SocketPublisherImpl.h"
#include "SocketUtil.h"

#include <zmq.h>

Common::ZeroMQWrapperImpl::SocketPublisherImpl::SocketPublisherImpl(Common::ZeroMQWrapperImpl::ContextHolder &context)
    : SocketImpl(context,ZMQ_PUB)
{
}

void Common::ZeroMQWrapperImpl::SocketPublisherImpl::write(const std::vector<std::string> &data)
{
    SocketUtil::write(m_socket,data);
}
