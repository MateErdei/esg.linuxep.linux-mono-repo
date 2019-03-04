/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ZeroMQWrapper/ISocketSubscriberPtr.h>
#include <Common/ZeroMQWrapperImpl/ContextHolder.h>
#include <Common/ZeroMQWrapperImpl/ProxyImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketHolder.h>
#include <Common/Reactor/IReactor.h>

namespace Common
{
    namespace ZMQWrapperApiImpl
    {
        class LoggingProxyImpl
                : public Common::ZeroMQWrapperImpl::ProxyImpl
        {
        public:
            LoggingProxyImpl(const std::string& frontend, const std::string& backend, Common::ZeroMQWrapperImpl::ContextHolderSharedPtr context);

            void start() override;

            void stop() override;

        private:
            std::string m_captureAddress;
            Common::ZeroMQWrapperImpl::SocketHolder m_capture;
            Common::ZeroMQWrapper::ISocketSubscriberPtr m_captureListener;
            std::unique_ptr<Common::Reactor::ICallbackListener> m_debugLoggerCallbackPtr;
            std::unique_ptr<Common::Reactor::IReactor> m_reactor;
        };
    } // namespace ZMQWrapperApiImpl
} // namespace Common