/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

#include "Common/Threads/AbstractThread.h"

namespace plugin::manager::scanprocessmonitor
{
    class ScanProcessMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ScanProcessMonitor(sophos_filesystem::path scanner_path = "");
        void run() override;
        void subprocess_exited();
    private:
        sophos_filesystem::path m_scanner_path;
        Common::Threads::NotifyPipe m_subprocess_terminated;
        Common::Threads::NotifyPipe m_config_changed;
    };
}



