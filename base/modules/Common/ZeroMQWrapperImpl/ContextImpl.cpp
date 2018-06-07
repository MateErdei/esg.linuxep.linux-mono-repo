//
// Created by pair on 06/06/18.
//

#include "ContextImpl.h"
#include "SocketRequesterImpl.h"

#include <Common/ZeroMQ_wrapper/ISocketSubscriber.h>
#include <Common/ZeroMQ_wrapper/ISocketPublisher.h>
#include <Common/ZeroMQ_wrapper/ISocketReplier.h>

#include <zmq.h>

using namespace Common::ZeroMQWrapperImpl;

std::unique_ptr<Common::ZeroMQ_wrapper::ISocketSubscriber> ContextImpl::getSubscriber()
{
    return std::unique_ptr<ZeroMQ_wrapper::ISocketSubscriber>();
}

std::unique_ptr<Common::ZeroMQ_wrapper::ISocketPublisher> ContextImpl::getPublisher()
{
    return std::unique_ptr<ZeroMQ_wrapper::ISocketPublisher>();
}

Common::ZeroMQ_wrapper::ISocketRequesterPtr ContextImpl::getRequester()
{
    return Common::ZeroMQ_wrapper::ISocketRequesterPtr(
            new SocketRequesterImpl(m_context)
            );
}

Common::ZeroMQ_wrapper::ISocketReplierPtr ContextImpl::getReplier()
{
    return Common::ZeroMQ_wrapper::ISocketReplierPtr();
}

std::unique_ptr<Common::ZeroMQ_wrapper::IContext> Common::ZeroMQ_wrapper::createContext()
{
    return std::unique_ptr<Common::ZeroMQ_wrapper::IContext>(new ContextImpl());
}
