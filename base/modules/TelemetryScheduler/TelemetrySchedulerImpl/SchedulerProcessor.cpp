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
    unsigned int GetIntervalFromSupplementaryJson(const nlohmann::json& j)
    {
        return j.contains("interval") ? (unsigned int)j.at("Interval") : 0;
    }

    void CreateTelemetryConfigJsonFile(
        const std::string& supplementaryJsonString,
        const std::string& configJasonFilepath)
    {
        Telemetry::TelemetryConfigImpl::Config config;
        Common::FileSystem::fileSystem()->writeFile(
            configJasonFilepath,
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
    }

    void SchedulerProcessor::run()
    {
        if (!Common::FileSystem::fileSystem()->isFile(m_supplementaryConfigFilepath))
        {
            LOGINFO("Supplementary file '" << m_supplementaryConfigFilepath << "' is not accessible");
        }
        else
        {
            std::string supplementaryConfigJson =
                Common::FileSystem::fileSystem()->readFile(m_supplementaryConfigFilepath, 1000000UL);
            m_interval = GetIntervalFromSupplementaryJson(nlohmann::json::parse(supplementaryConfigJson));
        }

        while (true)
        {
            auto task = m_taskQueue->pop();

            switch (task)
            {
                case Task::WaitToRunTelemetry:
                    waitToRunTelemetry(m_interval);
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

    void SchedulerProcessor::ProcessRunTelemetry()
    {
        std::string supplementaryConfigJson =
            Common::FileSystem::fileSystem()->readFile(m_supplementaryConfigFilepath, 1000000UL); // error checking here

        CreateTelemetryConfigJsonFile(supplementaryConfigJson, m_telemetryExeConfigFilepath);
        // TODO: LINUXEP-7984 run telelmetry executable
    }
} // namespace TelemetrySchedulerImpl
