/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/AbstractThreadPluginInterface.h"

#include <atomic>
#include <memory>

namespace common
{
    class ThreadRunner
    {
    public:
        explicit ThreadRunner(std::shared_ptr<common::AbstractThreadPluginInterface> thread, std::string name, bool startOnInitialisation);
        ~ThreadRunner();
        ThreadRunner(const ThreadRunner&) = delete;
        ThreadRunner& operator=(ThreadRunner const&) = delete;

        void requestStopIfNotStopped();
        void startIfNotStarted();

        inline bool isStarted() { return m_started; }

    private:
        void stopThread();
        void startThread();

        std::shared_ptr<common::AbstractThreadPluginInterface> m_thread;
        std::string m_name;
        std::atomic_bool m_started = false;
    };
}