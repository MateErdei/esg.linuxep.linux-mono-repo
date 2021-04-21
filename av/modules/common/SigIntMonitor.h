/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Threads/NotifyPipe.h>

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
        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}