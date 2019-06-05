/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "ITaskQueue.h"
#include "SchedulerStatus.h"
#include "SchedulerStatusSerialiser.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <json.hpp>
#include <thread>

namespace TelemetrySchedulerImpl
{
    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<ITaskQueue> taskQueue,
        const Common::ApplicationConfiguration::IApplicationPathManager& pathManager) :
        m_taskQueue(std::move(taskQueue)),
        m_pathManager(pathManager)
    {
        if (!m_taskQueue)
        {
            throw std::invalid_argument("precondition: taskQueue is not null failed");
        }
    }

    void SchedulerProcessor::run()
    {
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
                    break;

                case Task::CheckExecutableFinished:
                    checkExecutableFinished();
                    break;

                case Task::Shutdown:
                    return;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }

    size_t SchedulerProcessor::getScheduledTimeUsingSupplementaryFile()
    {
        if (!Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySupplementaryFilePath()))
        {
            LOGERROR(
                "Supplementary file '" << m_pathManager.getTelemetrySupplementaryFilePath() << "' is not accessible");
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
                    m_pathManager.getTelemetrySchedulerStatusFilePath(),
                    SchedulerStatusSerialiser::serialise(schedulerConfig));
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR(
                    "File access error writing " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : "
                                                 << e.what());
            }
        }

        return scheduledTimeInSecondsSinceEpoch;
    }

    size_t SchedulerProcessor::getIntervalFromSupplementaryFile()
    {
        std::string supplementaryConfigJson;

        try
        {
            supplementaryConfigJson = Common::FileSystem::fileSystem()->readFile(
                m_pathManager.getTelemetrySupplementaryFilePath(),
                Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "File access error reading " << m_pathManager.getTelemetrySupplementaryFilePath() << " : " << e.what());
            return 0;
        }

        try
        {
            const std::string intervalKey = "interval";
            nlohmann::json j = nlohmann::json::parse(supplementaryConfigJson);
            return j.contains(intervalKey) ? (size_t)j.at(intervalKey) : 0;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            LOGERROR("Supplementary file " << m_pathManager.getTelemetrySupplementaryFilePath()
                                           << " JSON is invalid: " << e.what(););
            return 0;
        }
    }

    void SchedulerProcessor::waitToRunTelemetry()
    {
        size_t scheduledTimeInSecondsSinceEpoch = 0;

        if (!Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySchedulerStatusFilePath()))
        {
            scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingSupplementaryFile();
        }
        else
        {
            std::string statusJsonString;
            SchedulerStatus schedulerConfig;

            try
            {
                statusJsonString = Common::FileSystem::fileSystem()->readFile(
                    m_pathManager.getTelemetrySchedulerStatusFilePath(),
                    Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE);
                schedulerConfig = SchedulerStatusSerialiser::deserialise(statusJsonString);
                scheduledTimeInSecondsSinceEpoch = schedulerConfig.getTelemetryScheduledTime();
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR(
                    "File access error reading " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : "
                                                 << e.what());
                scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingSupplementaryFile();
            }
            catch (const std::runtime_error& e)
            {
                LOGERROR(
                    "Invalid status file " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : " << e.what());

                try
                {
                    Common::FileSystem::fileSystem()->removeFile(m_pathManager.getTelemetrySchedulerStatusFilePath());
                }
                catch (const Common::FileSystem::IFileSystemException& e)
                {
                    LOGERROR(
                        "File access error removing " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : "
                                                      << e.what());
                }

                scheduledTimeInSecondsSinceEpoch =
                    getScheduledTimeUsingSupplementaryFile(); // should regenerate status file
            }
        }

        LOGDEBUG("Scheduled telemetry time: " << scheduledTimeInSecondsSinceEpoch);

        if (scheduledTimeInSecondsSinceEpoch == 0)
        {
            // Only run delay if not currently running.
            if (!m_delayBeforeCheckingConfiguration || m_delayBeforeCheckingConfiguration->finished())
            {
                using namespace std::chrono_literals;
                const std::chrono::seconds delay = 3600s; // TODO: breakout as configuration option?
                const auto now = std::chrono::system_clock::now();
                const auto timeToCheckConfiguration = now + delay;
                const size_t timeToCheckConfigurationSecondsSinceEpoch =
                    std::chrono::duration_cast<std::chrono::seconds>(timeToCheckConfiguration.time_since_epoch())
                        .count();
                LOGINFO(
                    "Telemetry reporting is currently disabled - will check again at "
                    << timeToCheckConfigurationSecondsSinceEpoch << " seconds since epoch");

                m_delayBeforeCheckingConfiguration = std::make_unique<SleepyThread>(
                    timeToCheckConfigurationSecondsSinceEpoch, Task::WaitToRunTelemetry, m_taskQueue);
                m_delayBeforeCheckingConfiguration->start();
            }
        }
        else if (!m_delayBeforeRunningTelemetry || m_delayBeforeRunningTelemetry->finished())
        {
            // Only run delay if not currently running.
            LOGINFO(
                "Telemetry reporting is scheduled to run at " << scheduledTimeInSecondsSinceEpoch
                                                              << " seconds since epoch");
            // For simplicity here, queue Task::RunTelemetry via delaying thread even if scheduled time is in the past.
            m_delayBeforeRunningTelemetry =
                std::make_unique<SleepyThread>(scheduledTimeInSecondsSinceEpoch, Task::RunTelemetry, m_taskQueue);
            m_delayBeforeRunningTelemetry->start();
        }
    }

    void SchedulerProcessor::runTelemetry()
    {
        LOGINFO("Telemetry reporting is about to run");

        std::string supplementaryConfigJson = Common::FileSystem::fileSystem()->readFile(
            m_pathManager.getTelemetrySupplementaryFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE); // error checking here

        Common::TelemetryExeConfigImpl::Config config;

        Common::FileSystem::fileSystem()->writeFile(
            m_pathManager.getTelemetryExeConfigFilePath(),
            Common::TelemetryExeConfigImpl::Serialiser::serialise(
                Common::TelemetryExeConfigImpl::Serialiser::deserialise(supplementaryConfigJson)));

        m_telemetryExeProcess = Common::Process::createProcess();

        try
        {
            m_telemetryExeProcess->setOutputLimit(Common::TelemetryExeConfigImpl::ONE_KBYTE_SIZE);
            m_telemetryExeProcess->exec(
                m_pathManager.getTelemetryExecutableFilePath(), { m_pathManager.getTelemetryExeConfigFilePath() });

            // Only run delay if not currently running.
            if (!m_delayBeforeCheckingConfiguration || m_delayBeforeCheckingConfiguration->finished())
            {
                using namespace std::chrono_literals;
                const std::chrono::seconds delay = 60s; // TODO: breakout as configuration option?
                const auto now = std::chrono::system_clock::now();
                const auto timeToCheckExeState = now + delay;
                const size_t timeToCheckExeStateSecondsSinceEpoch =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        timeToCheckExeState.time_since_epoch())
                        .count();

                m_delayBeforeCheckingExe = std::make_unique<SleepyThread>(
                    timeToCheckExeStateSecondsSinceEpoch, Task::CheckExecutableFinished, m_taskQueue);
                m_delayBeforeCheckingExe->start();
            }
        }
        catch (const Common::Process::IProcessException& processException)
        {
            LOGERROR("Failed to send telemetry: " << processException.what());
            m_taskQueue->push(Task::WaitToRunTelemetry);
        }
    }

    void SchedulerProcessor::checkExecutableFinished()
    {
        if (m_telemetryExeProcess->getStatus() != Common::Process::ProcessStatus::FINISHED)
        {
            m_telemetryExeProcess->kill();
            LOGERROR("Running telemetry executable timed out");
        }
        else
        {
            int exitCode = m_telemetryExeProcess->exitCode();
            if (exitCode != 0)
            {
                LOGERROR("Telemetry executable failed with exit code " << exitCode);
            }
        }

        m_taskQueue->push(Task::WaitToRunTelemetry);
    }

} // namespace TelemetrySchedulerImpl
