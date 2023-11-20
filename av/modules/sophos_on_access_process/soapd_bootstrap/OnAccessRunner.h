// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "OnAccessStatusFile.h"

#include "IOnAccessRunner.h"

#include "common/ThreadRunner.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/DeviceUtil.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"
#include "sophos_on_access_process/ScanRequestQueue/ScanRequestQueue.h"

#include <atomic>
#include <cassert>
#include <memory>
#include <utility>

namespace sophos_on_access_process::soapd_bootstrap
{
    class OnAccessRunner : public IOnAccessRunner
    {
    public:
        OnAccessRunner(
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCallWrapper,
            onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility) :
            m_sysCallWrapper(std::move(sysCallWrapper)), m_TelemetryUtility(std::move(telemetryUtility))
        {
            assert(m_sysCallWrapper);
            assert(m_TelemetryUtility);
        }

        void setupOnAccess() override;
        // these two methods are not thread safe, but are only called from ProcessPolicy and innerRun (after stopping
        // the policy handler)
        void enableOnAccess() override;
        void disableOnAccess() override;

        /**
         * Reads and uses the policy settings if m_policyOverride is false.
         * Called by OnAccessProcessControlCallback.
         * The function blocks on m_pendingConfigActionMutex to ensure that all actions it takes are thread safe.
         */
        void ProcessPolicy() override;

        timespec* getTimeout() override;
        void onTimeout() override;

        threat_scanner::IUpdateCompleteCallbackPtr getUpdateCompleteCallbackPtr() override
        {
            return m_fanotifyHandler;
        }

    private:
        bool getPolicyConfiguration(sophos_on_access_process::OnAccessConfig::OnAccessConfiguration& oaConfig);
        void applyConfig(const OnAccessConfig::OnAccessConfiguration& config);

        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  m_sysCallWrapper;
        onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr m_TelemetryUtility;

        sophos_on_access_process::local_settings::OnAccessLocalSettings m_localSettings;
        OnAccessConfig::OnAccessConfiguration m_config;
        std::mutex m_pendingConfigActionMutex;
        std::atomic_bool m_currentOaEnabledState{ false };
        soapd_bootstrap::OnAccessStatusFile m_statusFile;

        std::shared_ptr<fanotifyhandler::IFanotifyHandler> m_fanotifyHandler;
        std::shared_ptr<mount_monitor::mount_monitor::MountMonitor> m_mountMonitor;
        std::shared_ptr<common::ThreadRunner> m_mountMonitorThread;

        std::shared_ptr<onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::vector<std::shared_ptr<common::ThreadRunner>> m_scanHandlerThreads;
        std::shared_ptr<fanotifyhandler::EventReaderThread> m_eventReader;
        std::unique_ptr<common::ThreadRunner> m_eventReaderThread;
        mount_monitor::mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;
    };
} // namespace sophos_on_access_process::onaccessimpl
