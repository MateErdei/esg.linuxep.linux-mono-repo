///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once


#include "ContextHolder.h"
#include "SocketUtil.h"

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
            std::string m_controlAddress;
            ContextHolder m_context;
            std::thread m_thread;
            std::mutex m_threadStarted;
            bool m_threadStartedFlag;
            std::condition_variable m_ensureThreadStarted;
            SocketHolder m_controlPub;

            void announceThreadStarted();

        };
    }
}



