// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessConfigurationUtils.h"

#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include "common/ThreadRunner.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

#include <atomic>
#include <vector>

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        static int runSoapd();
        /**
         * Reads and uses the policy settings if m_policyOverride is false.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         *
         * @param onStart - true when we are starting soapd - log if disabling on-access when disabled
         *
         */
        void ProcessPolicy(bool onStart=false);

    private:
        int outerRun();

        void innerRun();

        void enableOnAccess(bool changed);
        void disableOnAccess(bool changed);

        sophos_on_access_process::OnAccessConfig::OnAccessConfiguration getPolicyConfiguration();

        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<sophos_on_access_process::fanotifyhandler::FanotifyHandler> m_fanotifyHandler;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;
        std::shared_ptr<common::ThreadRunner> m_mountMonitorThread;

        std::mutex m_pendingConfigActionMutex;
        std::atomic_bool m_currentOaEnabledState = false;
        std::atomic_bool m_onAccessEnabledFlag = false;


        std::shared_ptr<onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::vector<std::shared_ptr<common::ThreadRunner>> m_scanHandlerThreads;
    };
}