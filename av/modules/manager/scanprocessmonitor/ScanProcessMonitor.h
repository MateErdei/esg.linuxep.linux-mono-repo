// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ConfigMonitor.h"

#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"
#include "modules/common/NotifyPipeSleeper.h"
#include "scan_messages/ProcessControlSerialiser.h"

namespace plugin::manager::scanprocessmonitor
{
    class ScanProcessMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit ScanProcessMonitor(std::string processControllerSocket,
                                    datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
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
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;
        std::string m_processControllerSocketPath;
        std::shared_ptr<common::NotifyPipeSleeper> m_sleeper;
    };
}



