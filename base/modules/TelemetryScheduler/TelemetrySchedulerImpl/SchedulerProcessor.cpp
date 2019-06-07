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
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <json.hpp>
#include <thread>

namespace TelemetrySchedulerImpl
{
    using namespace std::chrono;

    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<ITaskQueue> taskQueue,
        const Common::ApplicationConfiguration::IApplicationPathManager& pathManager,
        seconds configurationCheckDelay,
        seconds telemetryExeCheckDelay) :
        m_taskQueue(std::move(taskQueue)),
        m_pathManager(pathManager),
        m_configurationCheckDelay(configurationCheckDelay),
        m_telemetryExeCheckDelay(telemetryExeCheckDelay)
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
                case SchedulerTask::InitialWaitToRunTelemetry:
                    waitToRunTelemetry(true);
                    break;

                case SchedulerTask::WaitToRunTelemetry:
                    waitToRunTelemetry(false);
                    break;

                case SchedulerTask::RunTelemetry:
                    runTelemetry();
                    break;

                case SchedulerTask::CheckExecutableFinished:
                    checkExecutableFinished();
                    break;

                case SchedulerTask::Shutdown:
                    return;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }

    size_t SchedulerProcessor::getScheduledTimeUsingIntervalFromSupplementaryFile(
        size_t previousTelemetryRunTimeInSecondsSinceEpoch)
    {
        if (!Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySupplementaryFilePath()))
        {
            LOGERROR(
                "Supplementary file '" << m_pathManager.getTelemetrySupplementaryFilePath() << "' is not accessible");
            return 0;
        }

        size_t interval = getIntervalFromSupplementaryFile();

        if (interval <= 0)
        {
            return 0;
        }

        auto duration = system_clock::duration(seconds(previousTelemetryRunTimeInSecondsSinceEpoch));
        system_clock::time_point previousScheduledTime(duration);

        system_clock::time_point scheduledTime = previousScheduledTime + seconds(interval);

        if (scheduledTime < system_clock::now())
        {
            scheduledTime = system_clock::now() + seconds(interval);
        }

        size_t scheduledTimeInSecondsSinceEpoch =
            duration_cast<seconds>(scheduledTime.time_since_epoch()).count();
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

        return scheduledTimeInSecondsSinceEpoch;
    }

    size_t SchedulerProcessor::getIntervalFromSupplementaryFile()
    {
        try
        {
            std::string supplementaryConfigJson = Common::FileSystem::fileSystem()->readFile(
                m_pathManager.getTelemetrySupplementaryFilePath(),
                Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE);

            const std::string intervalKey = "interval";
            nlohmann::json j = nlohmann::json::parse(supplementaryConfigJson);
            return j.contains(intervalKey) ? (size_t)j.at(intervalKey) : 0;
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "File access error reading " << m_pathManager.getTelemetrySupplementaryFilePath() << " : " << e.what());
            return 0;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (const nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            LOGERROR("Supplementary file " << m_pathManager.getTelemetrySupplementaryFilePath()
                                           << " JSON is invalid: " << e.what(););
            return 0;
        }
    }

    void SchedulerProcessor::delayBeforeQueueingTask(
        system_clock::time_point delayUntil,
        std::unique_ptr<SleepyThread>& delayThread,
        SchedulerTask task)
    {
        if (delayThread && !delayThread->finished())
        {
            return; // already running, so don't restart thread
        }

        delayThread = std::make_unique<SleepyThread>(delayUntil, task, m_taskQueue);
        delayThread->start();
    }

    void SchedulerProcessor::waitToRunTelemetry(bool runScheduledInPastNow)
    {
        // Always re-read values from configuration and status files in case they've been externally updated.

        size_t scheduledTimeInSecondsSinceEpoch = 0;

        if (!Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySchedulerStatusFilePath()))
        {
            scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingIntervalFromSupplementaryFile(0);
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

                if (!runScheduledInPastNow)
                {
                    scheduledTimeInSecondsSinceEpoch =
                        getScheduledTimeUsingIntervalFromSupplementaryFile(scheduledTimeInSecondsSinceEpoch);
                }
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR(
                    "File access error reading " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : "
                                                 << e.what());
                scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingIntervalFromSupplementaryFile(0);
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

                scheduledTimeInSecondsSinceEpoch = getScheduledTimeUsingIntervalFromSupplementaryFile(0);
            }
        }

        LOGDEBUG("Scheduled telemetry time: " << scheduledTimeInSecondsSinceEpoch);

        if (scheduledTimeInSecondsSinceEpoch == 0)
        {
            const auto timeToCheckConfiguration = system_clock::now() + m_configurationCheckDelay;

            LOGINFO(
                "Telemetry reporting is currently disabled - will check again in " << m_configurationCheckDelay.count()
                                                                                   << " seconds");

            delayBeforeQueueingTask(
                timeToCheckConfiguration, m_delayBeforeCheckingConfiguration, SchedulerTask::InitialWaitToRunTelemetry);
        }
        else
        {
            // Do this whether the time scheduled is in the past or in the future.

            auto duration = system_clock::duration(seconds(scheduledTimeInSecondsSinceEpoch));
            system_clock::time_point scheduledTime(duration);

            std::string formattedScheduledTime =
                Common::UtilityImpl::TimeUtils::fromTime(system_clock::to_time_t(scheduledTime));
            LOGINFO("Telemetry reporting is scheduled to run at " << formattedScheduledTime);

            delayBeforeQueueingTask(scheduledTime, m_delayBeforeRunningTelemetry, SchedulerTask::RunTelemetry);
        }
    }

    void SchedulerProcessor::runTelemetry()
    {
        LOGINFO("Telemetry reporting is about to run");

        try
        {
            std::string supplementaryConfigJson = Common::FileSystem::fileSystem()->readFile(
                m_pathManager.getTelemetrySupplementaryFilePath(),
                Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE); // error checking here

            Common::TelemetryExeConfigImpl::Config config;

            Common::FileSystem::fileSystem()->writeFile(
                m_pathManager.getTelemetryExeConfigFilePath(),
                Common::TelemetryExeConfigImpl::Serialiser::serialise(
                    Common::TelemetryExeConfigImpl::Serialiser::deserialise(supplementaryConfigJson)));

            m_telemetryExeProcess = Common::Process::createProcess();

            m_telemetryExeProcess->setOutputLimit(Common::TelemetryExeConfigImpl::MAX_OUTPUT_SIZE);
            m_telemetryExeProcess->exec(
                m_pathManager.getTelemetryExecutableFilePath(), { m_pathManager.getTelemetryExeConfigFilePath() });

            LOGINFO(
                "Telemetry executable's state will be checked in " << m_telemetryExeCheckDelay.count()
                                                                   << " seconds");
            const auto timeToCheckExeState = system_clock::now() + m_telemetryExeCheckDelay;
            delayBeforeQueueingTask(
                timeToCheckExeState, m_delayBeforeCheckingExe, SchedulerTask::CheckExecutableFinished);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "File access error reading " << m_pathManager.getTelemetrySupplementaryFilePath() << " or writing "
                                             << m_pathManager.getTelemetrySupplementaryFilePath() << ": " << e.what());
            m_taskQueue->push(SchedulerTask::WaitToRunTelemetry);
        }
        catch (const Common::Process::IProcessException& processException)
        {
            LOGERROR("Running telemetry executable failed: " << processException.what());
            m_taskQueue->push(SchedulerTask::WaitToRunTelemetry);
        }
        catch (const std::runtime_error& e)
        {
            std::stringstream msg;
            LOGERROR("Supplementary file " << m_pathManager.getTelemetrySupplementaryFilePath()
                                           << " is invalid: " << e.what(););
            m_taskQueue->push(SchedulerTask::WaitToRunTelemetry);
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
                LOGERROR("Telemetry executable output: " << m_telemetryExeProcess->output());
            }
        }

        m_taskQueue->push(SchedulerTask::WaitToRunTelemetry);
    }
} // namespace TelemetrySchedulerImpl