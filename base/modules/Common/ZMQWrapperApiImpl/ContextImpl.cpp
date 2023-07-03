/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ContextImpl.h"

#include "LoggingProxyImpl.h"

#include "modules/Common/ZeroMQWrapperImpl/ProxyImpl.h"
#include "modules/Common/ZeroMQWrapperImpl/SocketPublisherImpl.h"
#include "modules/Common/ZeroMQWrapperImpl/SocketReplierImpl.h"
#include "modules/Common/ZeroMQWrapperImpl/SocketRequesterImpl.h"
#include "modules/Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h"

#include "modules/Common/ZMQWrapperApi/IContextSharedPtr.h"

#include <iostream>

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
    auto envProxyLogging = ::secure_getenv("SOPHOS_PUB_SUB_ROUTER_LOGGING");
    if (envProxyLogging == nullptr)
    {
        return Common::ZeroMQWrapper::IProxyPtr(new Common::ZeroMQWrapperImpl::ProxyImpl(frontend, backend, m_context));
    }
    return Common::ZeroMQWrapper::IProxyPtr(
        new Common::ZMQWrapperApiImpl::LoggingProxyImpl(frontend, backend, m_context));
}

Common::ZMQWrapperApi::IContextSharedPtr Common::ZMQWrapperApi::createContext()
{
    return std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
}
