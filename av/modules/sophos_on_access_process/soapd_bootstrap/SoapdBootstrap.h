// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessConfigurationUtils.h"
#include "OnAccessStatusFile.h"
#include "OnAccessServiceImpl.h"

#include "common/ThreadRunner.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/DeviceUtil.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"
#include "sophos_on_access_process/local_settings/OnAccessLocalSettings.h"
#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryUtility.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include <atomic>
#include <vector>

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        explicit SoapdBootstrap(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
        SoapdBootstrap(const SoapdBootstrap&) =delete;
        SoapdBootstrap& operator=(const SoapdBootstrap&) =delete;

        static int runSoapd(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
        /**
         * Reads and uses the policy settings if m_policyOverride is false.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */
        void ProcessPolicy();

        static bool checkIfOAShouldBeEnabled(bool OnAccessEnabledFlag, bool OnAccessEnabledPolicySetting);

    private:
        int outerRun();

        void innerRun();

        void setupOnAccess();
        // these two methods are not thread safe, but are only called from ProcessPolicy and innerRun (after stopping the policy handler)
        void enableOnAccess();
        void disableOnAccess();

        bool getPolicyConfiguration(sophos_on_access_process::OnAccessConfig::OnAccessConfiguration& oaConfig);

        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<fanotifyhandler::IFanotifyHandler> m_fanotifyHandler;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;
        std::shared_ptr<common::ThreadRunner> m_mountMonitorThread;

        std::mutex m_pendingConfigActionMutex;
        std::atomic_bool m_currentOaEnabledState = false;

        sophos_on_access_process::local_settings::OnAccessLocalSettings m_localSettings;
        OnAccessConfig::OnAccessConfiguration m_config;

        std::shared_ptr<onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::vector<std::shared_ptr<common::ThreadRunner>> m_scanHandlerThreads;
        std::shared_ptr<fanotifyhandler::EventReaderThread> m_eventReader;
        mount_monitor::mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;

        std::shared_ptr<onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtility> m_TelemetryUtility = nullptr;
        std::unique_ptr<service_impl::OnAccessServiceImpl> m_ServiceImpl;
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;

        OnAccessStatusFile m_statusFile;
    };
}