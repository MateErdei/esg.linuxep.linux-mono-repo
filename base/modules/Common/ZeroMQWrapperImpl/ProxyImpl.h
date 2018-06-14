///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_PROXYIMPL_H
#define EVEREST_BASE_PROXYIMPL_H

#include "ContextHolder.h"

#include <Common/ZeroMQWrapper/IProxy.h>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ProxyImpl
            : public ZeroMQWrapper::IProxy
        {
        public:
            ProxyImpl(const std::string& frontend, const std::string& backend);
            ~ProxyImpl() override;

            void start() override;

            void stop() override;

            void run();
            ContextHolder &ctx();

        private:
            std::string m_frontendAddress;
            std::string m_backendAddress;
            ContextHolder m_context;
            std::thread m_thread;
            std::mutex m_threadStarted;
            std::condition_variable m_ensureThreadStarted;

            void announceThreadStarted();

        };
    }
}


#endif //EVEREST_BASE_PROXYIMPL_H
