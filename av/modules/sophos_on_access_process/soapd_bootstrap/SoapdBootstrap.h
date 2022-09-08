// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessConfigurationUtils.h"

#include "mount_monitor/mount_monitor/MountMonitor.h"

#include "common/ThreadRunner.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        static int runSoapd();
        /**
         * Performs all necessary actions for disabling the policy override.
         * Reads the policy settings from soapd_config.json and enables/disables OA accordingly.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */
        void UsePolicySettings();
        /**
         * Disables on-access despite the policy setting.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */
        void OverridePolicy();
        /**
         * Reads and uses the policy settings if m_policyOverride is false.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */
        void ProcessPolicy();

    private:
        void innerRun(
            std::shared_ptr<common::signals::SigIntMonitor>& sigIntMonitor,
            std::shared_ptr<common::signals::SigTermMonitor>& sigTermMonitor);

        sophos_on_access_process::OnAccessConfig::OnAccessConfiguration getConfiguration();

        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;

        std::mutex m_pendingConfigActionMutex;
        std::atomic<bool> m_policyOverride = true;
        std::atomic<bool> m_currentOaEnabledState = false;
    };
}
