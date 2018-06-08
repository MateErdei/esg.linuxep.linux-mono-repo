//
// Created by pair on 06/06/18.
//

#include "ContextImpl.h"
#include "SocketRequesterImpl.h"
#include "SocketReplierImpl.h"
#include "SocketPublisherImpl.h"
#include "SocketSubscriberImpl.h"

#include <Common/ZeroMQ_wrapper/ISocketSubscriber.h>

#include <zmq.h>

using namespace Common::ZeroMQWrapperImpl;

Common::ZeroMQ_wrapper::ISocketSubscriberPtr ContextImpl::getSubscriber()
{
    return Common::ZeroMQ_wrapper::ISocketSubscriberPtr(
            new SocketSubscriberImpl(m_context)
            );
}

Common::ZeroMQ_wrapper::ISocketPublisherPtr ContextImpl::getPublisher()
{
    return Common::ZeroMQ_wrapper::ISocketPublisherPtr(
            new SocketPublisherImpl(m_context)
            );
}

Common::ZeroMQ_wrapper::ISocketRequesterPtr ContextImpl::getRequester()
{
    return Common::ZeroMQ_wrapper::ISocketRequesterPtr(
            new SocketRequesterImpl(m_context)
            );
}

Common::ZeroMQ_wrapper::ISocketReplierPtr ContextImpl::getReplier()
{
    return Common::ZeroMQ_wrapper::ISocketReplierPtr(
            new SocketReplierImpl(m_context)
            );
}

std::unique_ptr<Common::ZeroMQ_wrapper::IContext> Common::ZeroMQ_wrapper::createContext()
{
    return std::unique_ptr<Common::ZeroMQ_wrapper::IContext>(new ContextImpl());
}
