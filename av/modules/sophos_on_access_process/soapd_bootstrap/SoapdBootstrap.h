// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessConfigurationUtils.h"

#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/DeviceUtil.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include "common/ThreadRunner.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/ZMQWrapperApi/IContext.h"

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
        void ProcessPolicy();

        static bool checkIfOAShouldBeEnabled(bool OnAccessEnabledFlag, bool OnAccessEnabledPolicySetting);

    private:
        int outerRun();

        void innerRun();

        void enableOnAccess();
        void disableOnAccess();

        void initialiseTelemetry();

        sophos_on_access_process::OnAccessConfig::OnAccessConfiguration getPolicyConfiguration();

        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<sophos_on_access_process::fanotifyhandler::FanotifyHandler> m_fanotifyHandler;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;
        std::shared_ptr<common::ThreadRunner> m_mountMonitorThread;

        std::mutex m_pendingConfigActionMutex;
        std::atomic_bool m_currentOaEnabledState = false;

        int m_maxNumberOfScanThreads = 0;
        bool m_dumpPerfData = false;

        std::shared_ptr<onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::vector<std::shared_ptr<common::ThreadRunner>> m_scanHandlerThreads;
        std::shared_ptr<::fanotifyhandler::EventReaderThread> m_eventReader;
        mount_monitor::mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;

        Common::ZMQWrapperApi::IContextSharedPtr m_onAccessContext = Common::ZMQWrapperApi::createContext();
        std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
    };
}