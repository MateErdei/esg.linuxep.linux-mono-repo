/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SocketSubscriberImpl.h"

#include "SocketUtil.h"
#include "ZeroMQWrapperException.h"

using namespace Common::ZeroMQWrapperImpl;

std::vector<std::string> SocketSubscriberImpl::read()
{
    return SocketUtil::read(m_socket);
}

void SocketSubscriberImpl::subscribeTo(const std::string& subject)
{
    int rc = zmq_setsockopt(m_socket.skt(), ZMQ_SUBSCRIBE, subject.data(), subject.size());
    if (rc != 0)
    {
        throw ZeroMQWrapperException("Failed to set subscription");
    }
}
