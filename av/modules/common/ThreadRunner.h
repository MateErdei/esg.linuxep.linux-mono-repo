/******************************************************************************************************

Copyright 2020-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/pluginimpl/Logger.h"
#include "Common/Threads/AbstractThread.h"

namespace common
{
    class ThreadRunner
    {
    public:
        explicit ThreadRunner(Common::Threads::AbstractThread &thread, std::string name)
                : m_thread(thread), m_name(std::move(name))
        {
            LOGINFO("Starting " << m_name);
            m_thread.start();
        }

        ~ThreadRunner()
        {
            killThreads();
        }

        void killThreads()
        {
            LOGINFO("Stopping " << m_name);
            m_thread.requestStop();
            LOGINFO("Joining " << m_name);
            m_thread.join();
        }

        Common::Threads::AbstractThread &m_thread;
        std::string m_name;
    };
}