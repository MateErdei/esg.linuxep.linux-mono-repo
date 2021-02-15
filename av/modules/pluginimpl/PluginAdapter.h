/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "AlcPolicyProcessor.h"
#include "PluginCallback.h"
#include "QueueTask.h"

#include "manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "manager/scheduler/ScanScheduler.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IBaseServiceApi.h>

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
        bool m_processedSAVPolicyBeforeLoop = false;
        bool m_processedALCPolicyBeforeLoop = false;
        int m_waitForPolicyTimeout = 0;

    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            int waitForPolicyTimeout = 5);
        void mainLoop();
        void processScanComplete(std::string& scanCompletedXml) override;
        void processThreatReport(const std::string& threatDetectedXML);

    private:
        void processPolicy(const std::string& policyXml);
        void processAction(const std::string& actionXml);
        void innerLoop();

        void incrementTelemetryThreatCount(const std::string &threatName);
        AlcPolicyProcessor m_updatePolicyProcessor;

        std::string waitForTheFirstPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS, int maxTasksThreshold,
                                          const std::string& policyAppId, const std::string& policyName);
    };
} // namespace Plugin
