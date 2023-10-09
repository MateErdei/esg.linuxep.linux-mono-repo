/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"
#include "Telemetry.h"

#include <Common/PluginApi/NoPolicyAvailableException.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");

        // If the plugin requires a mcs policy the plugin needs to explicitly request the policy on start-up
        // or the plugin will not receive the policy until the next time the policy changes.
        // the try catch code below uses the ALC policy as an example.
        try
        {
            // Request required policies for plugin.
            m_baseService->requestPolicies("ALC");
        }
        catch (const Common::PluginApi::NoPolicyAvailableException&)
        {
            LOGINFO("No policy available right now for app: " << "ALC");
            // Ignore no Policy Available errors
        }

        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.m_taskType)
            {
                case Task::TaskType::STOP:
                    return;

                case Task::TaskType::EXAMPLETASK:
                    break;

                case Task::TaskType::POLICY:
                    processPolicy(task.m_content);
                    break;
            }
        }
    }

    void PluginAdapter::processPolicy(const std::string& policyXml) { LOGDEBUG("Process policy: " << policyXml); }

    std::string PluginAdapter::waitForTheFirstPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS,
                                                     int maxTasksThreshold,
                                                     const std::string& policyAppId)
    {
        std::vector<Plugin::Task> nonPolicyTasks;
        std::string policyContent;
        for (int i = 0; i < maxTasksThreshold; i++)
        {
            Plugin::Task task;
            if (!queueTask.pop(task, timeoutInS.count()))
            {
                LOGINFO(policyAppId << " policy has not been sent to the plugin");
                break;
            }
            if (task.m_taskType == Plugin::Task::TaskType::POLICY && task.m_appId == policyAppId)
            {
                policyContent = task.m_content;
                LOGINFO("First " << policyAppId << " policy received.");
                break;
            }
            LOGSUPPORT("Keep task: " << static_cast<int>(task.m_taskType));
            nonPolicyTasks.push_back(task);
            if (task.m_taskType == Plugin::Task::TaskType::STOP)
            {
                LOGINFO("Abort waiting for the first policy as Stop signal received.");
                throw DetectRequestToStop("Stopping Plugin");
            }
        }
        LOGDEBUG("Return from waitForTheFirstPolicy ");
        for (const auto& task : nonPolicyTasks)
        {
            queueTask.push(task);
        }
        return policyContent;
    }
} // namespace Plugin