/******************************************************************************************************

Copyright 2018-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "PolicyProcessor.h"
#include "QueueTask.h"

#include "modules/common/ThreadRunner.h"
#include "manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "manager/scheduler/ScanScheduler.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>

namespace Plugin
{
    class PluginAdapter : public IScanComplete
    {
    private:
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        manager::scheduler::ScanScheduler m_scanScheduler;
        unixsocket::ThreatReporterServerSocket m_threatReporterServer;
        std::unique_ptr<plugin::manager::scanprocessmonitor::ScanProcessMonitor> m_threatDetector;
        int m_waitForPolicyTimeout = 0;

        Common::ZMQWrapperApi::IContextSharedPtr m_zmqContext;
        Common::ZeroMQWrapper::ISocketPublisherPtr m_threatEventPublisher;

    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            const std::string& threatEventPublisherSocketPath,
            int waitForPolicyTimeout = 5);
        void mainLoop();
        void processScanComplete(std::string& scanCompletedXml, int exitCode) override;
        void processThreatReport(const std::string& threatDetectedXML);
        void publishThreatEvent(const std::string& threatDetectedJSON);
        void publishThreatHealth(long threatStatus);
        void connectToThreatPublishingSocket(const std::string& pubSubSocketAddress);

    private:
        /**
         *
         * @param policyXml
         * @return true if a new policy has arrived false otherwise
         */
        bool processPolicy(const std::string& policyXml);
        void processAction(const std::string& actionXml);
        void startThreads();
        void innerLoop();
        void processSUSIRestartRequest();
        static void incrementTelemetryThreatCount(const std::string &threatName);

        PolicyProcessor m_policyProcessor;
        bool m_restartSophosThreatDetector = false;

        bool m_gotSavPolicy = false;
        bool m_gotAlcPolicy = false;

        std::unique_ptr<common::ThreadRunner> m_schedulerThread;
        std::unique_ptr<common::ThreadRunner> m_threatDetectorThread;
    };
} // namespace Plugin
