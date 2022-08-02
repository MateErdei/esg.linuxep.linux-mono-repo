// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "signals/SignalHandlerBase.h"

#include <memory>

namespace common
{
    class SigIntMonitor
    {
    public:
        explicit SigIntMonitor();
        ~SigIntMonitor();

        SigIntMonitor(const SigIntMonitor&) = delete;
        SigIntMonitor& operator=(const SigIntMonitor&) = delete;
        static std::shared_ptr<SigIntMonitor> getSigIntMonitor();
        int monitorFd()
        {
            return m_pipe.readFd();
        }
        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}