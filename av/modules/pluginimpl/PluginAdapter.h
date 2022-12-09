// Copyright 2018-2022 Sophos Limited. All rights reserved.

#pragma once

#ifndef PLUGIN_INTERNAL
#    define PLUGIN_INTERNAL private
#endif

#define ENABLE_CORE_POLICY
// #define ENABLE_CORC_POLICY

#include "DetectionQueue.h"
#include "HealthStatus.h"
#include "IDetectionDatabaseHandler.h"
#include "IDetectionReportProcessor.h"
#include "IRestoreReportProcessor.h"
#include "PluginCallback.h"
#include "PolicyProcessor.h"
#include "PolicyWaiter.h"
#include "SafeStoreWorker.h"
#include "TaskQueue.h"
#include "ThreatDatabase.h"

#include "manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "manager/scheduler/ScanScheduler.h"
#include "modules/common/ThreadRunner.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingServer.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"

namespace Plugin
{
    constexpr std::chrono::seconds DUPLICATE_DETECTION_TIMEOUT {60};

    class PluginAdapter : public IScanComplete,
                          public IDetectionReportProcessor,
                          public IDetectionDatabaseHandler,
                          public IRestoreReportProcessor
    {
    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        std::shared_ptr<DetectionQueue> m_detectionQueue;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::shared_ptr<manager::scheduler::ScanScheduler> m_scanScheduler;
        std::shared_ptr<unixsocket::ThreatReporterServerSocket> m_threatReporterServer;
        std::shared_ptr<plugin::manager::scanprocessmonitor::ScanProcessMonitor> m_threatDetector;
        std::shared_ptr<SafeStoreWorker> m_safeStoreWorker;
        unixsocket::RestoreReportingServer m_restoreReportingServer;
        int m_waitForPolicyTimeout = 0;

        Common::ZMQWrapperApi::IContextSharedPtr m_zmqContext;
        Common::ZeroMQWrapper::ISocketPublisherPtr m_threatEventPublisher;

    public:
        PluginAdapter(
            std::shared_ptr<TaskQueue> taskQueue,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            const std::string& threatEventPublisherSocketPath,
            int waitForPolicyTimeout = 5);
        void mainLoop();
        void processScanComplete(std::string& scanCompletedXml) override;

        /*
         * Takes in detection info and the result from attempting to quarantine that threat and then
         * triggers various outputs to be generated: Central Events, Event Journal input, Threat Health
         */
        void processDetectionReport(
            const scan_messages::ThreatDetected&,
            const common::CentralEnums::QuarantineResult& quarantineResult) const override;

        void publishThreatEvent(const std::string& threatDetectedJSON) const;
        void updateThreatDatabase(const scan_messages::ThreatDetected& detection) override;
        void connectToThreatPublishingSocket(const std::string& pubSubSocketAddress);
        bool isSafeStoreEnabled() const;
        bool shouldSafeStoreQuarantineMl() const;
        [[nodiscard]] std::shared_ptr<DetectionQueue> getDetectionQueue() const;
        void processRestoreReport(const scan_messages::RestoreReport& restoreReport) const override;

        static const PolicyWaiter::policy_list_t m_requested_policies;

        // clang-format off
    PLUGIN_INTERNAL:
        ; // Needed to prevent clang-format from being silly
        // clang-format on
        void publishThreatHealth(E_HEALTH_STATUS threatStatus) const;
        void publishThreatHealthWithRetry(E_HEALTH_STATUS threatStatus) const;
        static void incrementTelemetryThreatCount(
            const std::string& threatName,
            const scan_messages::E_SCAN_TYPE& scanType);

    private:
        /**
         *
         * @param policyXml
         * @param policyWaiter - Object that stores whether we've received all policies or not
         * @param appId OUT - the appId for the policy
         */
        void processPolicy(const std::string& policyXml, const PolicyWaiterSharedPtr& policyWaiter, const std::string& appId);
        void processAction(const std::string& actionXml);
        void startThreads();
        void innerLoop();
        void processSUSIRestartRequest();
        void setResetThreatDetector(bool reset)
        {
            m_restartSophosThreatDetector = reset || m_restartSophosThreatDetector;
        }

        PolicyProcessor m_policyProcessor;
        ThreatDatabase m_threatDatabase;
        bool m_restartSophosThreatDetector = false;

        std::unique_ptr<common::ThreadRunner> m_safeStoreWorkerThread;
        std::unique_ptr<common::ThreadRunner> m_schedulerThread;
        std::unique_ptr<common::ThreadRunner> m_threatDetectorThread;
        void processFlags(const std::string& flagsJson);
    };
} // namespace Plugin
