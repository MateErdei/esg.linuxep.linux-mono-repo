/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ContextImpl.h"

#include "Common/ZeroMQWrapperImpl/ProxyImpl.h"
#include "Common/ZeroMQWrapperImpl/SocketPublisherImpl.h"
#include "Common/ZeroMQWrapperImpl/SocketReplierImpl.h"
#include "Common/ZeroMQWrapperImpl/SocketRequesterImpl.h"
#include "Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h"

#include <iostream>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>

using namespace Common::ZMQWrapperApiImpl;

ContextImpl::ContextImpl() : m_context(new Common::ZeroMQWrapperImpl::ContextHolder()) {}

ContextImpl::~ContextImpl()
{
    if (m_context.use_count() > 1)
    {
        std::cerr << "WARNING: Deleting m_context while use_count > 1: " << m_context.use_count() << std::endl;
    }
}

Common::ZeroMQWrapper::ISocketSubscriberPtr ContextImpl::getSubscriber()
{
    return Common::ZeroMQWrapper::ISocketSubscriberPtr(new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(m_context));
}

Common::ZeroMQWrapper::ISocketPublisherPtr ContextImpl::getPublisher()
{
    return Common::ZeroMQWrapper::ISocketPublisherPtr(new Common::ZeroMQWrapperImpl::SocketPublisherImpl(m_context));
}

Common::ZeroMQWrapper::ISocketRequesterPtr ContextImpl::getRequester()
{
    return Common::ZeroMQWrapper::ISocketRequesterPtr(new Common::ZeroMQWrapperImpl::SocketRequesterImpl(m_context));
}

Common::ZeroMQWrapper::ISocketReplierPtr ContextImpl::getReplier()
{
    return Common::ZeroMQWrapper::ISocketReplierPtr(new Common::ZeroMQWrapperImpl::SocketReplierImpl(m_context));
}

Common::ZeroMQWrapper::IProxyPtr ContextImpl::getProxy(const std::string& frontend, const std::string& backend)
{
    return Common::ZeroMQWrapper::IProxyPtr(new Common::ZeroMQWrapperImpl::ProxyImpl(frontend, backend, m_context));
}

Common::ZMQWrapperApi::IContextSharedPtr Common::ZMQWrapperApi::createContext()
{
    return std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
}
