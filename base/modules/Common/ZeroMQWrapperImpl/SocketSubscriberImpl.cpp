//
// Created by pair on 08/06/18.
//

#include <cassert>
#include "SocketSubscriberImpl.h"
#include "SocketUtil.h"

using namespace Common::ZeroMQWrapperImpl;

std::vector<std::string> SocketSubscriberImpl::read()
{
    return SocketUtil::read(m_socket);
}

void SocketSubscriberImpl::subscribeTo(const std::string &subject)
{
    int rc = zmq_setsockopt(m_socket.skt(),ZMQ_SUBSCRIBE,subject.data(),subject.size());
    assert(rc == 0);
}
