// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "signals/SignalHandlerBase.h"

#include <memory>

namespace common
{
    class SigTermMonitor : public common::signals::SignalHandlerBase
    {
    public:
        explicit SigTermMonitor();
        SigTermMonitor(const SigTermMonitor&) = delete;
        ~SigTermMonitor();

        int monitorFd();
        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}