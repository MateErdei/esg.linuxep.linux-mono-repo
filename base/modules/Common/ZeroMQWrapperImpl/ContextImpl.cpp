/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ContextImpl.h"
#include "SocketRequesterImpl.h"
#include "SocketReplierImpl.h"
#include "SocketPublisherImpl.h"
#include "SocketSubscriberImpl.h"

using namespace Common::ZeroMQWrapperImpl;

Common::ZeroMQWrapper::ISocketSubscriberPtr ContextImpl::getSubscriber()
{
    return Common::ZeroMQWrapper::ISocketSubscriberPtr(
            new SocketSubscriberImpl(m_context)
            );
}

Common::ZeroMQWrapper::ISocketPublisherPtr ContextImpl::getPublisher()
{
    return Common::ZeroMQWrapper::ISocketPublisherPtr(
            new SocketPublisherImpl(m_context)
            );
}

Common::ZeroMQWrapper::ISocketRequesterPtr ContextImpl::getRequester()
{
    return Common::ZeroMQWrapper::ISocketRequesterPtr(
            new SocketRequesterImpl(m_context)
            );
}

Common::ZeroMQWrapper::ISocketReplierPtr ContextImpl::getReplier()
{
    return Common::ZeroMQWrapper::ISocketReplierPtr(
            new SocketReplierImpl(m_context)
            );
}

Common::ZeroMQWrapper::IContextSharedPtr Common::ZeroMQWrapper::createContext()
{
    return std::make_shared<Common::ZeroMQWrapperImpl::ContextImpl>();
}
