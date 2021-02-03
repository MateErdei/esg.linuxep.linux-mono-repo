/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unixsocket/IMonitorable.h>

#include <Common/Threads/NotifyPipe.h>

namespace avscanner
{
    class SigIntMonitor
    {
    public:
        explicit SigIntMonitor();
        SigIntMonitor(const SigIntMonitor&) = delete;
        ~SigIntMonitor();

        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}