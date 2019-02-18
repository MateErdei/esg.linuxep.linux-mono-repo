/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketPublisherImpl.h"

#include "SocketUtil.h"

#include <zmq.h>

Common::ZeroMQWrapperImpl::SocketPublisherImpl::SocketPublisherImpl(
    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr context) :
    SocketImpl(std::move(context), ZMQ_PUB)
{
    // the linger controls that the publisher will be kept running on its destructor to give
    // time to the subscriber to receive the remaining messages.
    // http://api.zeromq.org/4-2:zmq-ctx-term
    SocketUtil::setConnectionTimeout(m_socket, 1000);
}

void Common::ZeroMQWrapperImpl::SocketPublisherImpl::write(const std::vector<std::string>& data)
{
    SocketUtil::write(m_socket, data);
}
