/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Threads/AbstractThread.h"

namespace plugin::manager::scanprocessmonitor
{
    class ScanProcessMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ScanProcessMonitor(std::string scanner_path = "");
        void run() override;
        void subprocess_exited();
    private:
        std::string m_scanner_path;
        Common::Threads::NotifyPipe m_subprocess_terminated;
    };
}



