/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Threads/NotifyPipe.h>

#include <memory>

namespace common
{
    class SigTermMonitor
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