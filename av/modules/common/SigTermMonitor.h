/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unixsocket/IMonitorable.h>

#include <Common/Threads/NotifyPipe.h>

namespace common
{
    class SigTermMonitor
    {
    public:
        explicit SigTermMonitor();
        SigTermMonitor(const SigTermMonitor&) = delete;
        ~SigTermMonitor();
        static std::shared_ptr<SigTermMonitor> makeSigTermMonitor();
        int monitorFd();
        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}