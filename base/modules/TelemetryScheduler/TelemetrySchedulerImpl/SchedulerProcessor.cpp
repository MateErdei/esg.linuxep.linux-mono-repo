/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "SchedulerStatusSerialiser.h"
#include "SchedulerTask.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/OSUtilitiesImpl/SXLMachineID.h>
#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/TelemetryConfigImpl/Serialiser.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

namespace
{
    constexpr unsigned int dayInSeconds(86400);
}

namespace TelemetrySchedulerImpl
{
    using namespace Common::TelemetryConfigImpl;
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
            throw std::invalid_argument("Precondition (taskQueue is not null) failed");
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

    std::tuple<SchedulerStatus, bool> SchedulerProcessor::getStatusFromFile() const
    {
        if (Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySchedulerStatusFilePath()))
        {
            try
            {
                std::string statusJsonString = Common::FileSystem::fileSystem()->readFile(
                    m_pathManager.getTelemetrySchedulerStatusFilePath(), DEFAULT_MAX_JSON_SIZE);
                SchedulerStatus schedulerStatus = SchedulerStatusSerialiser::deserialise(statusJsonString);
                return std::make_tuple(schedulerStatus, true);
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR(
                    "File access error reading " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : "
                                                 << e.what());
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
            }
        }

        return std::make_tuple(SchedulerStatus{}, false);
    }

    std::tuple<Config, bool> SchedulerProcessor::getConfigFromFile() const
    {
        if (Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySupplementaryFilePath()))
        {
            try
            {
                std::string telemetryConfigJson = Common::FileSystem::fileSystem()->readFile(
                    m_pathManager.getTelemetrySupplementaryFilePath(), DEFAULT_MAX_JSON_SIZE);
                Config telemetryConfig = Serialiser::deserialise(telemetryConfigJson);
                LOGDEBUG(
                    "Using configuration: " << Common::TelemetryConfigImpl::Serialiser::serialise(telemetryConfig));
                return std::make_tuple(telemetryConfig, true);
            }
            catch (const Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR(
                    "File access error reading telemetry configuration file "
                    << m_pathManager.getTelemetrySupplementaryFilePath() << " : " << e.what());
            }
            catch (const std::runtime_error& e)
            {
                LOGERROR("Telemetry configuration file " << m_pathManager.getTelemetrySupplementaryFilePath()
                                                         << " JSON is invalid: " << e.what(););
            }
        }

        return std::make_tuple(Config{}, false);
    }

    void SchedulerProcessor::updateStatusFile(const system_clock::time_point& scheduledTime) const
    {
        SchedulerStatus schedulerStatus;
        schedulerStatus.setTelemetryScheduledTime(scheduledTime);

        try
        {
            Common::FileSystem::fileSystem()->writeFile(
                m_pathManager.getTelemetrySchedulerStatusFilePath(),
                SchedulerStatusSerialiser::serialise(schedulerStatus));
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "File access error writing " << m_pathManager.getTelemetrySchedulerStatusFilePath() << " : "
                                             << e.what());
        }
    }

    system_clock::time_point SchedulerProcessor::getNextScheduledTime(
        system_clock::time_point previousScheduledTime,
        unsigned int intervalSeconds) const
    {
        const system_clock::time_point epoch;
        system_clock::time_point scheduledTime = epoch;

        scheduledTime = previousScheduledTime + seconds(intervalSeconds);

        if (scheduledTime < system_clock::now())
        {
            scheduledTime = system_clock::now() + seconds(intervalSeconds);
        }

        return scheduledTime;
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

    bool SchedulerProcessor::isTelemetryDisabled(
        const system_clock::time_point& previousScheduledTime,
        bool statusFileValid,
        unsigned int interval,
        bool configFileValid)
    {
        // Telemetry can be disabled remotely by setting the interval to zero. Telemetry can be disabled locally be
        // setting the scheduled time in the status file to the epoch.

        const system_clock::time_point epoch;

        return (statusFileValid && previousScheduledTime == epoch) || !configFileValid || interval == 0;
    }

    void SchedulerProcessor::waitToRunTelemetry(bool runScheduledInPastNow)
    {
        // Always re-read values from the telemetry configuration (supplementary) and status files in case they've been
        // externally updated.

        auto const& [schedulerStatus, statusFileValid] = getStatusFromFile();
        auto const& [telemetryConfig, configFileValid] = getConfigFromFile();
        auto previousScheduledTime = schedulerStatus.getTelemetryScheduledTime();
        auto interval = telemetryConfig.getInterval();

        if (isTelemetryDisabled(previousScheduledTime, statusFileValid, interval, configFileValid))
        {
            LOGINFO(
                "Telemetry reporting is currently disabled - will check again in " << m_configurationCheckDelay.count()
                                                                                   << " seconds");
            delayBeforeQueueingTask(
                system_clock::now() + m_configurationCheckDelay,
                m_delayBeforeCheckingConfiguration,
                SchedulerTask::InitialWaitToRunTelemetry);
            return;
        }

        const system_clock::time_point epoch;
        system_clock::time_point scheduledTime;

        // This logic is dependent on the logic that checks whether telemetry is disabled.
        if (statusFileValid && (previousScheduledTime > system_clock::now() || runScheduledInPastNow))
        {
            scheduledTime = previousScheduledTime;
        }
        else
        {
            scheduledTime = getNextScheduledTime(previousScheduledTime, interval);
            updateStatusFile(scheduledTime);
        }

        assert(scheduledTime != epoch);

        // Do the following whether the time scheduled is in the past or in the future.

        std::string formattedScheduledTime =
            Common::UtilityImpl::TimeUtils::fromTime(system_clock::to_time_t(scheduledTime));
        LOGINFO("Telemetry reporting is scheduled to run at " << formattedScheduledTime);

        delayBeforeQueueingTask(scheduledTime, m_delayBeforeRunningTelemetry, SchedulerTask::RunTelemetry);
    }

    void SchedulerProcessor::runTelemetry()
    {
        // Always re-read values from the telemetry configuration (supplementary) and status files in case they've been
        // externally updated.

        auto const& [schedulerStatus, statusFileValid] = getStatusFromFile();
        auto const& [telemetryConfig, configFileValid] = getConfigFromFile();
        auto previousScheduledTime = schedulerStatus.getTelemetryScheduledTime();
        auto interval = telemetryConfig.getInterval();

        if (isTelemetryDisabled(previousScheduledTime, statusFileValid, interval, configFileValid))
        {
            m_taskQueue->push(SchedulerTask::WaitToRunTelemetry);
            return;
        }

        LOGINFO("Telemetry reporting is about to run");

        try
        {
            Common::OSUtilitiesImpl::SXLMachineID sxlMachineId;
            std::string machineId = sxlMachineId.getMachineID();

            if (machineId.empty())
            {
                LOGINFO(
                    "No machine id for reporting telemetry - will check again in " << m_configurationCheckDelay.count()
                                                                                   << " seconds");
                delayBeforeQueueingTask(
                    system_clock::now() + m_configurationCheckDelay,
                    m_delayBeforeCheckingConfiguration,
                    SchedulerTask::InitialWaitToRunTelemetry);
                return;
            }

            machineId.erase(
                std::remove_if(
                    machineId.begin(), machineId.end(), [](auto const& c) -> bool { return !std::isalnum(c); }),
                machineId.end());

            std::string resourceName = machineId + ".json";

            Config telemetryExeConfig =
                Common::TelemetryConfigImpl::Config::buildExeConfigFromTelemetryConfig(telemetryConfig, resourceName);

            Common::FileSystem::fileSystem()->writeFile(
                m_pathManager.getTelemetryExeConfigFilePath(), Serialiser::serialise(telemetryExeConfig));

            m_telemetryExeProcess = Common::Process::createProcess();

            m_telemetryExeProcess->setOutputLimit(MAX_OUTPUT_SIZE);
            m_telemetryExeProcess->exec(
                m_pathManager.getTelemetryExecutableFilePath(), { m_pathManager.getTelemetryExeConfigFilePath() });

            LOGINFO(
                "Telemetry executable's state will be checked in " << m_telemetryExeCheckDelay.count() << " seconds");
            const auto timeToCheckExeState = system_clock::now() + m_telemetryExeCheckDelay;
            delayBeforeQueueingTask(
                timeToCheckExeState, m_delayBeforeCheckingExe, SchedulerTask::CheckExecutableFinished);
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("File access error writing " << m_pathManager.getTelemetryExeConfigFilePath() << ": " << e.what());
            m_taskQueue->push(SchedulerTask::WaitToRunTelemetry);
        }
        catch (const Common::Process::IProcessException& processException)
        {
            LOGERROR("Running telemetry executable failed: " << processException.what());
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