// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef PLUGIN_INTERNAL
# define PLUGIN_INTERNAL private
#endif

#include "HealthStatus.h"
#include "PluginCallback.h"
#include "PolicyProcessor.h"
#include "QueueSafeStoreTask.h"
#include "QueueTask.h"
#include "SafeStoreWorker.h"
#include "IDetectionReportProcessor.h"

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
        std::shared_ptr<QueueTask> m_queueTask;
        std::shared_ptr<QueueSafeStoreTask> m_queueSafeStoreTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::shared_ptr<manager::scheduler::ScanScheduler> m_scanScheduler;
        std::shared_ptr<unixsocket::ThreatReporterServerSocket> m_threatReporterServer;
        std::shared_ptr<plugin::manager::scanprocessmonitor::ScanProcessMonitor> m_threatDetector;
        std::shared_ptr<SafeStoreWorker> m_safeStoreQueueWorker;
        int m_waitForPolicyTimeout = 0;

        Common::ZMQWrapperApi::IContextSharedPtr m_zmqContext;
        Common::ZeroMQWrapper::ISocketPublisherPtr m_threatEventPublisher;

    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::shared_ptr<QueueSafeStoreTask> queueSafeStoreTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            const std::string& threatEventPublisherSocketPath,
            int waitForPolicyTimeout = 5);
        void mainLoop();
        void processScanComplete(std::string& scanCompletedXml, int exitCode) override;
        void processDetectionReport(const scan_messages::ServerThreatDetected&);
        void processThreatReport(const std::string& threatDetectedXML);
        void publishThreatEvent(const std::string& threatDetectedJSON);
        void connectToThreatPublishingSocket(const std::string& pubSubSocketAddress);
        bool isSafeStoreEnabled();
        std::shared_ptr<QueueSafeStoreTask> getSafeStoreQueue() const;

    PLUGIN_INTERNAL:
        void publishThreatHealth(E_HEALTH_STATUS threatStatus);
    private:
        /**
         *
         * @param policyXml
         * @param policyUpdated OUT - has the policy been updated
         * @param appId OUT - the appId for the policy
         */
        void processPolicy(const std::string& policyXml, bool& policyUpdated, std::string& appId);
        void processAction(const std::string& actionXml);
        void startThreads();
        void innerLoop();
        void processSUSIRestartRequest();
        static void incrementTelemetryThreatCount(const std::string &threatName);

        PolicyProcessor m_policyProcessor;
        bool m_restartSophosThreatDetector = false;

        bool m_gotSavPolicy = false;
        bool m_gotAlcPolicy = false;

        std::unique_ptr<common::ThreadRunner> m_safeStoreQueueWorkerThread;
        std::unique_ptr<common::ThreadRunner> m_schedulerThread;
        std::unique_ptr<common::ThreadRunner> m_threatDetectorThread;
        void processFlags(const std::string& flagsJson);
    };
} // namespace Plugin
