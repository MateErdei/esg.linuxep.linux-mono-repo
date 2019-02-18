/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketRequesterImpl.h"

#include "SocketUtil.h"

#include <zmq.h>

using namespace Common::ZeroMQWrapperImpl;

SocketRequesterImpl::SocketRequesterImpl(Common::ZeroMQWrapperImpl::ContextHolderSharedPtr context) :
    SocketImpl(std::move(context), ZMQ_REQ)
{
}

std::vector<std::string> Common::ZeroMQWrapperImpl::SocketRequesterImpl::read()
{
    try
    {
        int timeout_ = timeout();
        if (timeout_ != -1)
        {
            SocketUtil::checkIncomingData(m_socket, timeout_);
        }
        return SocketUtil::read(m_socket);
    }
    catch (std::exception&)
    {
        // it is necessary to refresh the socket on read exception otherwise the socket will not be able to perform any
        // other request.
        refresh();
        throw;
    }
}

void Common::ZeroMQWrapperImpl::SocketRequesterImpl::write(const std::vector<std::string>& data)
{
    SocketUtil::write(m_socket, data);
}
