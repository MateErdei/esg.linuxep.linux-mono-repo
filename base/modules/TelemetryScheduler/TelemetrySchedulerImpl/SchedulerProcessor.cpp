/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "SchedulerStatus.h"
#include "SchedulerStatusSerialiser.h"
#include "SchedulerTask.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <json.hpp>

namespace TelemetrySchedulerImpl
{
    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<TaskQueue> taskQueue,
        const std::string& supplementaryConfigFilePath,
        const std::string& telemetryExeConfigFilePath,
        const std::string& telemetryStatusFilePath) :
        m_taskQueue(std::move(taskQueue)),
        m_telemetryExeConfigFilePath(telemetryExeConfigFilePath),
        m_supplementaryConfigFilePath(supplementaryConfigFilePath),
        m_telemetryStatusFilePath(telemetryStatusFilePath)
    {
        if (!m_taskQueue)
        {
            throw std::invalid_argument("precondition: taskQueue is not null failed");
        }
    }

    void SchedulerProcessor::run()
    {
        waitToRunTelemetry();

        while (true)
        {
            auto task = m_taskQueue->pop();

            switch (task)
            {
                case Task::WaitToRunTelemetry:
                    waitToRunTelemetry();
                    break;

                case Task::RunTelemetry:
                    runTelemetry();
                    break; // TODO: LINUXEP-7984 run telelmetry executable

                case Task::Shutdown:
                    return;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }

    size_t SchedulerProcessor::getScheduledTimeUsingSupplementaryFile()
    {
        if (!Common::FileSystem::fileSystem()->isFile(m_supplementaryConfigFilePath))
        {
            LOGERROR("Supplementary file '" << m_supplementaryConfigFilePath << "' is not accessible");
            m_taskQueue->push(Task::WaitToRunTelemetry);
            return 0;
        }

        size_t interval = getIntervalFromSupplementaryFile();
        size_t scheduledTimeInSecondsSinceEpoch = 0;

        if (interval > 0)
        {
            auto scheduledTime = std::chrono::system_clock::now() + std::chrono::seconds(interval);
            scheduledTimeInSecondsSinceEpoch =
                std::chrono::duration_cast<std::chrono::seconds>(scheduledTime.time_since_epoch()).count();
            SchedulerStatus schedulerConfig;
            schedulerConfig.setTelemetryScheduledTime(scheduledTimeInSecondsSinceEpoch);

            try
            {
                Common::FileSystem::fileSystem()->writeFile(
                    m_telemetryStatusFilePath, SchedulerStatusSerialiser::serialise(schedulerConfig));
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("File access error writing " << m_telemetryExeConfigFilePath << " : " << e.what());
                // TODO: attempt to remove status file?
            }
        }

        return scheduledTimeInSecondsSinceEpoch;
    }

    size_t SchedulerProcessor::getIntervalFromSupplementaryFile()
    {
        try
        {
            const std::string intervalKey = "interval";
            std::string supplementaryConfigJson =
                Common::FileSystem::fileSystem()->readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE);
            nlohmann::json j = nlohmann::json::parse(supplementaryConfigJson);
            return j.contains(intervalKey) ? (size_t)j.at(intervalKey) : 0;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            LOGERROR("Supplementary file " << m_supplementaryConfigFilePath << " JSON is invalid: " << e.what(););
            return 0;
        }
    }

    void SchedulerProcessor::waitToRunTelemetry()
    {
        size_t scheduledTimeInSecondsSinceEpoch = 0;

        if (!Common::FileSystem::fileSystem()->isFile(m_telemetryStatusFilePath))
        {
            scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingSupplementaryFile();
        }
        else
        {
            std::string statusJsonString;

            try
            {
                statusJsonString = Common::FileSystem::fileSystem()->readFile(
                    Common::ApplicationConfiguration::applicationPathManager().getTelemetrySchedulerStatusFilePath(),
                    Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE);
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("File access error reading " << m_telemetryStatusFilePath << " : " << e.what());
                scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingSupplementaryFile();
            }

            SchedulerStatus schedulerConfig;

            try
            {
                schedulerConfig = SchedulerStatusSerialiser::deserialise(statusJsonString);
            }
            catch (const std::runtime_error& e)
            {
                // invalid status file, so remove it then regenerate

                try
                {
                    Common::FileSystem::fileSystem()->removeFile(m_telemetryStatusFilePath);
                }
                catch (const Common::FileSystem::IFileSystemException& e)
                {
                    LOGERROR("File access error removing " << m_telemetryStatusFilePath << " : " << e.what());
                    scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingSupplementaryFile();
                }
            }

            scheduledTimeInSecondsSinceEpoch = schedulerConfig.getTelemetryScheduledTime();
        }

        LOGDEBUG("Scheduled telemetry time: " << scheduledTimeInSecondsSinceEpoch);

        // TODO: LINUXEP-6639 if scheduledTimeInSecondsSinceEpoch == 0 then delay (diff delay than below, 1 hour say)
        // then send WaitToRunTelemetry
        // TODO: LINUXEP-6639 if scheduledTimeInSecondsSinceEpoch in past, send RunTelemetry now
        // TODO: LINUXEP-6639 if scheduledTimeInSecondsSinceEpoch in future, delay to scheduled time before sending
        // RunTelemetry
    }

    void SchedulerProcessor::runTelemetry()
    {
        std::string supplementaryConfigJson = Common::FileSystem::fileSystem()->readFile(
            m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE); // error checking here

        Common::TelemetryExeConfigImpl::Config config;

        Common::FileSystem::fileSystem()->writeFile(
            m_telemetryExeConfigFilePath,
            Common::TelemetryExeConfigImpl::Serialiser::serialise(
                Common::TelemetryExeConfigImpl::Serialiser::deserialise(supplementaryConfigJson)));

        // TODO: LINUXEP-7984 run telemetry executable
    }
} // namespace TelemetrySchedulerImpl
