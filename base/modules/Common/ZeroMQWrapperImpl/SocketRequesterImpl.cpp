/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketRequesterImpl.h"
#include "SocketUtil.h"

#include <zmq.h>

using namespace Common::ZeroMQWrapperImpl;

SocketRequesterImpl::SocketRequesterImpl(Common::ZeroMQWrapperImpl::ContextHolder &context)
    : SocketImpl(context, ZMQ_REQ)
{
}

std::vector<std::string> Common::ZeroMQWrapperImpl::SocketRequesterImpl::read()
{
    return SocketUtil::read(m_socket);
}

void Common::ZeroMQWrapperImpl::SocketRequesterImpl::write(const std::vector<std::string> &data)
{
    SocketUtil::write(m_socket,data);
}
