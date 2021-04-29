/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Threads/NotifyPipe.h>

#include <memory>

namespace common
{
    class SigHupMonitor
    {
    public:
        explicit SigHupMonitor();
        ~SigHupMonitor();

        SigHupMonitor(const SigHupMonitor&) = delete;
        SigHupMonitor& operator=(const SigHupMonitor&) = delete;

        static std::shared_ptr<SigHupMonitor> getSigHupMonitor();
        int monitorFd();
        bool triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}