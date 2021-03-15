/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unixsocket/IMonitorable.h>

#include <Common/Threads/NotifyPipe.h>

namespace common
{
    class SigIntMonitor
    {
    public:
        explicit SigIntMonitor();
        SigIntMonitor(const SigIntMonitor&) = delete;
        ~SigIntMonitor();

        static std::shared_ptr<SigIntMonitor> makeSigIntMonitor();
        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}