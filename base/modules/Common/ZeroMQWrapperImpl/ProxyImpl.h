/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ContextHolder.h"
#include "SocketHolder.h"
#include "SocketUtil.h"

#include <Common/ZeroMQWrapper/IProxy.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ProxyImpl : public ZeroMQWrapper::IProxy
        {
        public:
            ProxyImpl(const std::string& frontend, const std::string& backend, ContextHolderSharedPtr context);
            ~ProxyImpl() override;

            void start() override;

            void stop() override;

            void run();

            /**
             * Method used by unit tests to get the context used by the proxy,
             * so that we can use inproc addresses to access the proxy.
             * @return
             */
            ContextHolderSharedPtr& ctx();

        private:
            std::string m_frontendAddress;
            std::string m_backendAddress;
            std::string m_controlAddress;
            ContextHolderSharedPtr m_context;
            std::thread m_thread;
            std::mutex m_threadStarted;
            bool m_threadStartedFlag;
            std::condition_variable m_ensureThreadStarted;
            SocketHolder m_controlPub;

            void announceThreadStarted();

        protected:
            void* m_captureZMQSocket; // Used in derived class LoggerProxyImpl to
                                      // add a capture socket to zmq_proxy_steerable
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
