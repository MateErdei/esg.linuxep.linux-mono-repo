/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "QueueTask.h"

#include "manager/scheduler/ScanScheduler.h"
#include "manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

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
        std::unique_ptr<plugin::manager::scanprocessmonitor::ScanProcessMonitor> m_sophosThreadDetector;
        std::string m_lastPolicyRevID;

    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback);
        void mainLoop();
        void processScanComplete(std::string& scanCompletedXml) override;
        void processThreatReport(const std::string& threatDetectedXML);

    private:
        void processPolicy(const std::string& policyXml);
        void processAction(const std::string& actionXml);
        void innerLoop();
        std::string generateSAVStatusXML(const std::string& revID);

    };
} // namespace Plugin
