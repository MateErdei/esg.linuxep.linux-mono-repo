/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigMonitor.h"

#include "datatypes/sophos_filesystem.h"

namespace plugin::manager::scanprocessmonitor
{
    class ScanProcessMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ScanProcessMonitor(sophos_filesystem::path scanner_path = "");
        void run() override;
        void subprocess_exited();

        /**
         * Externally drive a configuration change event.
         * Causes sophos_threat_detector to be restarted.
         */
        void configuration_changed();
    private:
        sophos_filesystem::path m_scanner_path;
        Common::Threads::NotifyPipe m_subprocess_terminated;
        Common::Threads::NotifyPipe m_config_changed;
        ConfigMonitor m_config_monitor;
    };
}



