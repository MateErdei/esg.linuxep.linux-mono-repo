/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "SchedulerConfig.h"
#include "SchedulerTask.h"
#include "Serialiser.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <json.hpp>

namespace TelemetrySchedulerImpl
{
    unsigned int GetIntervalFromSupplementaryJson(const nlohmann::json& j)
    {
        return j.contains("interval") ? (unsigned int)j.at("Interval") : 0;
    }

    void CreateTelemetryConfigJsonFile(
        const std::string& supplementaryJsonString,
        const std::string& configJsonFilepath)
    {
        Telemetry::TelemetryConfigImpl::Config config;
        Common::FileSystem::fileSystem()->writeFile(
            configJsonFilepath,
            Telemetry::TelemetryConfigImpl::Serialiser::serialise(
                Telemetry::TelemetryConfigImpl::Serialiser::deserialise(supplementaryJsonString)));
    }

    void waitToRunTelemetry(int /* interval */)
    {
        // TODO: LINUXEP-6639 handle scheduling of next run of telemetry executable
    }

    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<TaskQueue> taskQueue,
        const std::string& supplementaryConfigFilepath,
        const std::string& telemetryExeConfigFilepath) :
        m_taskQueue(std::move(taskQueue)),
        m_telemetryExeConfigFilepath(telemetryExeConfigFilepath),
        m_supplementaryConfigFilepath(supplementaryConfigFilepath)

    {
        if (!m_taskQueue)
        {
            throw std::invalid_argument("precondition: taskQueue is not null failed");
        }
    }

    void SchedulerProcessor::run()
    {
        SchedulerConfig schedulerConfig;
        if (!Common::FileSystem::fileSystem()->isFile(
                Common::ApplicationConfiguration::applicationPathManager().getTelemetrySchedulerStatusFilePath()))
        {
            if (!Common::FileSystem::fileSystem()->isFile(m_supplementaryConfigFilepath))
            {
                LOGINFO("Supplementary file '" << m_supplementaryConfigFilepath << "' is not accessible");
            }
            else
                m_interval = GetIntervalFromSupplementaryJson();
        }
        else
        {
            std::string statusJsonString = Common::FileSystem::fileSystem()->readFile(
                Common::ApplicationConfiguration::applicationPathManager().getTelemetrySchedulerStatusFilePath(),
                1000000UL);

            schedulerConfig = Serialiser::deserialise(statusJsonString);
        }

        while (true)
        {
            auto task = m_taskQueue->pop();

            switch (task)
            {
                case Task::WaitToRunTelemetry:
                    waitToRunTelemetry(schedulerConfig.getTelemetryScheduledTime());
                    break;

                case Task::RunTelemetry:
                    ProcessRunTelemetry();
                    break; // TODO: LINUXEP-7984 run telelmetry executable

                case Task::Shutdown:
                    return;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }
    unsigned int SchedulerProcessor::GetIntervalFromSupplementaryJson()
    {
        std::string supplementaryConfigJson =
            Common::FileSystem::fileSystem()->readFile(m_supplementaryConfigFilepath, 1000000UL);
        nlohmann::json j = nlohmann::json::parse(supplementaryConfigJson);
        return j.contains("interval") ? (unsigned int)j.at("Interval") : 0;
    }

    void SchedulerProcessor::ProcessRunTelemetry()
    {
        std::string supplementaryConfigJson =
            Common::FileSystem::fileSystem()->readFile(m_supplementaryConfigFilepath, 1000000UL); // error checking here

        CreateTelemetryConfigJsonFile(supplementaryConfigJson, m_telemetryExeConfigFilepath);
        // TODO: LINUXEP-7984 run telelmetry executable
    }
} // namespace TelemetrySchedulerImpl
