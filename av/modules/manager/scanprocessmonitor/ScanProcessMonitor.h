/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigMonitor.h"

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"

namespace plugin::manager::scanprocessmonitor
{
    class ScanProcessMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit ScanProcessMonitor(std::string processControllerSocket);
        void run() override;

        /**
         * Externally drive a configuration change event.
         * Causes sophos_threat_detector to reload susi config.
         */
        void policy_configuration_changed();
    private:
        void sendRequestToThreatDetector(scan_messages::E_COMMAND_TYPE requestType);
        Common::Threads::NotifyPipe m_config_changed;
        Common::Threads::NotifyPipe m_policy_changed;
        ConfigMonitor m_config_monitor;
        std::string m_processControllerSocket;
    };
}



