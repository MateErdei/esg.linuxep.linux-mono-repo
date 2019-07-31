/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketPublisherImpl.h"

#include "SocketUtil.h"

#include <zmq.h>

// Note if we wanted to track the arrival of subscriptions we could use an XPUB socket with
// the following socket option
//
// zmq_setsockopt(m_socket.skt(), ZMQ_XPUB_MANUAL, 1, size_of(int));
//
// This would not set subscriptions automatically and you can listen for them by setting up
// a Reactor listening on the XPUB socket.
// When handling a subscription message you could then log the subscription and then call
//
// LOGDUBUG(<subscription string>);
// zmq_setsockopt(m_socket.skt(), ZMQ_SUBSCRIBE, <subscription string>, <length of subscription string>);
//
// The socket would then publish messages of the <subscription string> type and we would have
// a log of when the subscription arrived. Note you would also have to manually handle unsubscribes too.

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
