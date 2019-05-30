/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "SchedulerTask.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <json.hpp>

namespace TelemetrySchedulerImpl
{
    int GetIntervalFromSupplementaryJson(const nlohmann::json& j)
    {
        if (j.contains("interval"))
        {
            return j.at("Interval");
        }
    }

    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<TaskQueue> taskQueue,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> pluginCallback) :
        m_taskQueue(std::move(taskQueue)),
        m_baseService(std::move(baseService)),
        m_pluginCallback(std::move(pluginCallback))
    {
    }

    void SchedulerProcessor::run()
    {
        std::string supplementaryConfigFilePath(
            Common::ApplicationConfiguration::applicationPathManager().getTelemetrySupplementaryFilePath());

        int interval;
        if (!Common::FileSystem::fileSystem()->isFile(supplementaryConfigFilePath))
        {
            LOGINFO("Supplementary file '" << supplementaryConfigFilePath << "' is not accessible");
            interval = 86400;
        }
        else
        {
            std::string telemetryConfigJson =
                Common::FileSystem::fileSystem()->readFile(supplementaryConfigFilePath, 1000000UL);
            interval = GetIntervalFromSupplementaryJson(telemetryConfigJson);
        }
        // do something with interval

        while (true)
        {
            auto task = m_taskQueue->pop();

            switch (task.taskType)
            {
                case Task::TaskType::ShutdownReceived:
                    continue;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }
} // namespace TelemetrySchedulerImpl
