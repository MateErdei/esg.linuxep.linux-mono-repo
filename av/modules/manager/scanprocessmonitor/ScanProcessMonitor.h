// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ConfigMonitor.h"

#include "common/LockableData.h"
#include "datatypes/sophos_filesystem.h"
#include "common/NotifyPipeSleeper.h"
#include "scan_messages/ProcessControlSerialiser.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

namespace plugin::manager::scanprocessmonitor
{
    class ScanProcessMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit ScanProcessMonitor(std::string processControllerSocket,
                                    Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper);
        void run() override;

        /**
         * Externally drive a configuration change event.
         * Causes sophos_threat_detector to reload susi config.
         */
        void policy_configuration_changed();
        /**
         * Externally drive a configuration change event.
         * Causes sophos_threat_detector to restart.
         */
        void restart_threat_detector();

        void setSXL4LookupsEnabled(bool isEnabled);

    private:
        void sendRequestToThreatDetector(scan_messages::E_COMMAND_TYPE requestType);
        bool getSXL4LookupsEnabled();
        Common::Threads::NotifyPipe m_config_changed;
        Common::Threads::NotifyPipe m_policy_changed;
        Common::Threads::NotifyPipe restart_threat_detector_pipe_;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCallWrapper;
        std::string m_processControllerSocketPath;
        std::shared_ptr<common::NotifyPipeSleeper> m_sleeper;
        common::LockableData<bool> sxl4enabled_;
    };
}



