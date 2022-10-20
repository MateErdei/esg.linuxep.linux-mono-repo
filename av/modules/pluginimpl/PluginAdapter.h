// Copyright 2018-2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef PLUGIN_INTERNAL
# define PLUGIN_INTERNAL private
#endif

#include "DetectionQueue.h"
#include "HealthStatus.h"
#include "IDetectionReportProcessor.h"
#include "PluginCallback.h"
#include "PolicyProcessor.h"
#include "PolicyWaiter.h"
#include "SafeStoreWorker.h"
#include "TaskQueue.h"

#include "manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "manager/scheduler/ScanScheduler.h"
#include "modules/common/ThreadRunner.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>

namespace Plugin
{
    class PluginAdapter : public IScanComplete, public IDetectionReportProcessor
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
        void processDetectionReport(const scan_messages::ThreatDetected&) const override;
        void processThreatReport(const std::string& threatDetectedXML) const;
        void publishThreatEvent(const std::string& threatDetectedJSON) const;
        void connectToThreatPublishingSocket(const std::string& pubSubSocketAddress);
        bool isSafeStoreEnabled();
        [[nodiscard]] std::shared_ptr<DetectionQueue> getDetectionQueue() const;

    PLUGIN_INTERNAL:
        void publishThreatHealth(E_HEALTH_STATUS threatStatus) const;
    private:
        /**
         *
         * @param policyXml
         * @param appId OUT - the appId for the policy
         * @return whether policy has been updated
         */
        void processPolicy(const std::string& policyXml, PolicyWaiterSharedPtr policyWaiter);
        void processAction(const std::string& actionXml);
        void startThreads();
        void innerLoop();
        void processSUSIRestartRequest();
        static void incrementTelemetryThreatCount(const std::string &threatName);
        void setResetThreatDetector(bool reset) { m_restartSophosThreatDetector = reset || m_restartSophosThreatDetector; }

        PolicyProcessor m_policyProcessor;
        bool m_restartSophosThreatDetector = false;

        std::unique_ptr<common::ThreadRunner> m_safeStoreWorkerThread;
        std::unique_ptr<common::ThreadRunner> m_schedulerThread;
        std::unique_ptr<common::ThreadRunner> m_threatDetectorThread;
        void processFlags(const std::string& flagsJson);
    };
} // namespace Plugin
