// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessConfigurationUtils.h"

#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/DeviceUtil.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/local_settings/OnAccessLocalSettings.h"
#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryUtility.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include "common/ThreadRunner.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

#include "Common/PluginApiImpl/PluginCallBackHandler.h"
#include "Common/ZMQWrapperApi/IContext.h"

#include <atomic>
#include <optional>
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
         * @return The flag settings if flag file is present
         *
         */
        std::optional<bool> ProcessPolicy();

        static bool checkIfOAShouldBeEnabled(bool OnAccessEnabledFlag, bool OnAccessEnabledPolicySetting);

    private:
        int outerRun();

        void innerRun();

        void enableOnAccess();
        void disableOnAccess();

        void initialiseTelemetry();

        bool getPolicyConfiguration(sophos_on_access_process::OnAccessConfig::OnAccessConfiguration& oaConfig);

        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<sophos_on_access_process::fanotifyhandler::FanotifyHandler> m_fanotifyHandler;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;
        std::shared_ptr<common::ThreadRunner> m_mountMonitorThread;

        std::mutex m_pendingConfigActionMutex;
        std::atomic_bool m_currentOaEnabledState = false;

        sophos_on_access_process::local_settings::OnAccessLocalSettings m_localSettings;

        std::shared_ptr<onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::vector<std::shared_ptr<common::ThreadRunner>> m_scanHandlerThreads;
        std::shared_ptr<::fanotifyhandler::EventReaderThread> m_eventReader;
        mount_monitor::mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;

        Common::ZMQWrapperApi::IContextSharedPtr m_onAccessContext = Common::ZMQWrapperApi::createContext();
        std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler = nullptr;
        std::shared_ptr<onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtility> m_TelemetryUtility = nullptr;
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;
    };
}