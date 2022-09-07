// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"

#include "common/ThreadRunner.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        static int runSoapd();
        void UsePolicySettings();
        void OverridePolicy();
        void ProcessPolicy();

    private:
        void innerRun(
            std::shared_ptr<common::signals::SigIntMonitor>& sigIntMonitor,
            std::shared_ptr<common::signals::SigTermMonitor>& sigTermMonitor);

        sophos_on_access_process::OnAccessConfig::OnAccessConfiguration getConfiguration();

        //TODO: ensure thread safety
        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;

        std::atomic<bool> m_policyOverride = true;
        std::atomic<bool> m_currentOaEnabledState = false;
    };
}
