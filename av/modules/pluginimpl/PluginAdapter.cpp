/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"
#include "Telemetry.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/UtilityImpl/StringUtils.h>
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

using namespace Plugin;

namespace
{
    fs::path pluginInstall()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        return appConfig.getData("PLUGIN_INSTALL");
    }

    fs::path threat_reporter_socket()
    {
        return pluginInstall() / "chroot/threat_report_socket";
    }

    fs::path sophos_threat_detector_launcher()
    {
        return pluginInstall() /  "sbin/sophos_threat_detector_launcher";
    }

    class ThreadRunner
    {
    public:
        explicit ThreadRunner(Common::Threads::AbstractThread& thread, std::string name)
                : m_thread(thread), m_name(std::move(name))
        {
            LOGINFO("Starting " << m_name);
            m_thread.start();
        }

        ~ThreadRunner()
        {
            LOGINFO("Stopping " << m_name);
            m_thread.requestStop();
            LOGINFO("Joining " << m_name);
            m_thread.join();
        }

    private:
        Common::Threads::AbstractThread& m_thread;
        std::string m_name;
    };

    class ThreatReportCallbacks : public IMessageCallback
    {
    public:
        explicit ThreatReportCallbacks(PluginAdapter& adapter)
                :m_adapter(adapter)
        {

        }

        void processMessage(const std::string& threatXML) override
        {
            m_adapter.processThreatReport(threatXML);
        }
    private:
        PluginAdapter& m_adapter;
    };
}

PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_scanScheduler(*this),
        m_threatReporterServer(threat_reporter_socket(),  std::make_shared<ThreatReportCallbacks>(*this)),
        m_lastPolicyRevID("noPolicyReceived")
{

    m_sophosThreadDetector = std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            sophos_threat_detector_launcher()
    );
}

void PluginAdapter::mainLoop()
{
    LOGINFO("Entering the main loop");
    ThreadRunner scheduler(m_scanScheduler, "scanScheduler"); // Automatically terminate scheduler on both normal exit and exceptions
    ThreadRunner sophos_threat_reporter(m_threatReporterServer, "threatReporter");
    ThreadRunner sophos_thread_detector(*m_sophosThreadDetector, "threatDetector");
    innerLoop();
}

void PluginAdapter::innerLoop()
{
    while (true)
    {
        Task task = m_queueTask->pop();
        switch (task.taskType)
        {
            case Task::TaskType::Stop:
                return;

            case Task::TaskType::Policy:
                processPolicy(task.Content);
                break;

            case Task::TaskType::Action:
                processAction(task.Content);
                break;

            case Task::TaskType::ScanComplete:
                m_baseService->sendEvent("SAV", task.Content);
                break;

            case Task::TaskType::ThreatDetected:
                m_baseService->sendEvent("SAV", task.Content);
                break;

            case Task::TaskType::SendStatus:
                m_baseService->sendStatus("SAV", task.Content, task.Content);
                break;
        }
    }
}

std::string PluginAdapter::generateSAVStatusXML(const std::string& revID)
{
    std::string result = Common::UtilityImpl::StringUtils::orderedStringReplace(
            R"sophos(<?xml version="1.0" encoding="utf-8"?>
<status xmlns="http://www.sophos.com/EE/EESavStatus">
  <csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="Same" RevID="@@REV_ID@@" policyType="2"/>
  <upToDateState>1</upToDateState>
  <vdl-info>
    <virus-engine-version>N/A</virus-engine-version>
    <virus-data-version>N/A</virus-data-version>
    <idelist>
    </idelist>
    <ideChecksum>N/A</ideChecksum>
  </vdl-info>
  <on-access>false</on-access>
  <entity>
    <productId>SSPL-AV</productId>
    <product-version>N/A</product-version>
    <entityInfo>SSPL-AV</entityInfo>
  </entity>
</status>)sophos",{
                    {"@@REV_ID@@", revID}
            });

    return result;
}

void PluginAdapter::processPolicy(const std::string& policyXml)
{
    LOGDEBUG("Process policy: " << policyXml);

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    m_scanScheduler.updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));

    std::string revID = attributeMap.lookup("config/csc:Comp").value("RevID", "unknown");
    if (revID != m_lastPolicyRevID)
    {
        LOGDEBUG("Received new policy with revision ID: " << revID);
        m_queueTask->push(Task{.taskType=Task::TaskType::SendStatus, generateSAVStatusXML(revID)});
        m_lastPolicyRevID = revID;
    }
}

void PluginAdapter::processAction(const std::string& actionXml)
{
    LOGDEBUG("Process action: " << actionXml);

    auto attributeMap = Common::XmlUtilities::parseXml(actionXml);

    if (attributeMap.lookup("a:action").value("type", "") == "ScanNow")
    {
        m_scanScheduler.scanNow();
    }
}

void PluginAdapter::processScanComplete(std::string& scanCompletedXml)
{
    LOGDEBUG("Sending scan complete notification to central: " << scanCompletedXml);

    m_queueTask->push(Task{.taskType=Task::TaskType::ScanComplete, scanCompletedXml});
}

void PluginAdapter::processThreatReport(const std::string& threatDetectedXML)
{
    LOGDEBUG("Sending threat detection notification to central: " << threatDetectedXML);

    m_queueTask->push(Task{.taskType=Task::TaskType::ThreatDetected, threatDetectedXML});
}
