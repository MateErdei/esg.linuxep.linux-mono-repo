// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "OnAccessConfigurationUtils.h"
#include "OnAccessStatusFile.h"
#include "IOnAccessService.h"
#include "ISoapdResources.h"

#include "IPolicyProcessor.h"

#include "common/ThreadRunner.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/DeviceUtil.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"
#include "sophos_on_access_process/local_settings/OnAccessLocalSettings.h"
#include "sophos_on_access_process/onaccessimpl/IOnAccessTelemetryUtility.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include <atomic>
#include <vector>

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap : public IPolicyProcessor
    {
    public:
        explicit SoapdBootstrap(ISoapdResources& soapdResources);
        SoapdBootstrap(const SoapdBootstrap&) =delete;
        SoapdBootstrap& operator=(const SoapdBootstrap&) =delete;

        static int runSoapd();
        /**
         * Reads and uses the policy settings if m_policyOverride is false.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */
        void ProcessPolicy() override;

        static bool checkIfOAShouldBeEnabled(bool OnAccessEnabledFlag, bool OnAccessEnabledPolicySetting);

TEST_PUBLIC:
        int outerRun();

    private:
        void innerRun();

        void setupOnAccess();
        // these two methods are not thread safe, but are only called from ProcessPolicy and innerRun (after stopping the policy handler)
        void enableOnAccess();
        void disableOnAccess();

        bool getPolicyConfiguration(sophos_on_access_process::OnAccessConfig::OnAccessConfiguration& oaConfig);

        ISoapdResources& m_soapdResources;

        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        std::shared_ptr<fanotifyhandler::IFanotifyHandler> m_fanotifyHandler;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;
        std::shared_ptr<common::ThreadRunner> m_mountMonitorThread;

        std::mutex m_pendingConfigActionMutex;
        std::atomic_bool m_currentOaEnabledState{ false };

        sophos_on_access_process::local_settings::OnAccessLocalSettings m_localSettings;
        OnAccessConfig::OnAccessConfiguration m_config;

        std::shared_ptr<onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::vector<std::shared_ptr<common::ThreadRunner>> m_scanHandlerThreads;
        std::shared_ptr<fanotifyhandler::EventReaderThread> m_eventReader;
        mount_monitor::mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;

        onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr m_TelemetryUtility{ nullptr };
        service_impl::IOnAccessServicePtr m_ServiceImpl;
        datatypes::ISystemCallWrapperSharedPtr m_sysCallWrapper;

        OnAccessStatusFile m_statusFile;
    };
}