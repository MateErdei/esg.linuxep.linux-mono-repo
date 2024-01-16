// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "SchedulerProcessor.h"

#include "RequestToStopException.h"
#include "SchedulerStatusSerialiser.h"
#include "SchedulerTask.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Exceptions/NestedException.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/OSUtilitiesImpl/SXLMachineID.h"
#include "Common/Policy/ALCPolicy.h"
#include "Common/Policy/PolicyParseException.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"
#include "Common/TelemetryConfigImpl/Serialiser.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "TelemetryScheduler/LoggerImpl/Logger.h"

#include <cassert>

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

            switch (task.taskType)
            {
                case SchedulerTask::TaskType::InitialWaitToRunTelemetry:
                    waitToRunTelemetry(true);
                    break;

                case SchedulerTask::TaskType::WaitToRunTelemetry:
                    waitToRunTelemetry(false);
                    break;

                case SchedulerTask::TaskType::RunTelemetry:
                    try
                    {
                        runTelemetry();
                    }
                    catch (const RequestToStopException& ex)
                    {
                        LOGINFO("Have not received first ALC policy yet so not running Telemetry: " << ex.what());
                    }
                    break;

                case SchedulerTask::TaskType::CheckExecutableFinished:
                    checkExecutableFinished();
                    break;

                case SchedulerTask::TaskType::Shutdown:
                    return;

                case SchedulerTask::TaskType::Policy:
                    if (task.appId == "ALC")
                    {
                        LOGDEBUG("Processing " << task.appId << " policy: " << task.content);
                        processALCPolicy(task.content);
                    }
                    else
                    {
                        LOGWARN("Received unexpected " << task.appId << " policy: " << task.content);
                    }
                    break;

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
                LOGERROR(
                    "Telemetry configuration file " << m_pathManager.getTelemetrySupplementaryFilePath()
                                                    << " JSON is invalid: " << e.what());
            }
        }

        return std::make_tuple(Config{}, false);
    }

    void SchedulerProcessor::saveStatusFile(const SchedulerStatus& schedulerStatus) const
    {
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

    void SchedulerProcessor::saveLastStartTimeStatus(const time_point& startTime) const
    {
        auto const& [schedulerStatusTemp, schedulerStatusFileValid] = getStatusFromFile();
        assert(schedulerStatusFileValid); // Can't serialise if not valid
        auto schedulerStatus = schedulerStatusTemp;
        schedulerStatus.setLastTelemetryStartTime(startTime);
        saveStatusFile(schedulerStatus);
    }

    SchedulerProcessor::time_point SchedulerProcessor::getNextScheduledTime(
        time_point previousScheduledTime,
        unsigned int intervalSeconds,
        time_point now
        )
    {
        system_clock::time_point scheduledTime = previousScheduledTime + seconds(intervalSeconds);

        if (scheduledTime < now)
        {
            scheduledTime = now + seconds(intervalSeconds);
        }

        return scheduledTime;
    }

    SchedulerProcessor::time_point SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(
            const std::optional<SchedulerStatus> schedulerStatus,
            bool newInstall,
            unsigned int interval,
            bool runScheduledInPastNow,
            system_clock::time_point timePointNow
    )
    {
        if (schedulerStatus)
        {
            auto lastStart = schedulerStatus.value().getLastTelemetryStartTime();
            if (lastStart)
            {
                // Normal case - we recorded when we last started telemetry,
                // run interval amount later
                return lastStart.value() + seconds{interval};
            }
            else
            {
                // No last start time - upgrade
                auto previousScheduledTime = schedulerStatus.value().getTelemetryScheduledTime();
                if (runScheduledInPastNow)
                {
                    // at start of scheduler execution after upgrade, so run at the specified time
                    // even if in the last
                    // runScheduledInPastNow means that if telemetry status has a time which is in the past, we should run telemetry
                    // immediately, rather than select a new time in the future and wait for that.
                    // This logic is dependent on the logic that checks whether telemetry is disabled.
                    return previousScheduledTime;
                }
                else if (previousScheduledTime > timePointNow)
                {
                    // Scheduled time is still in the future, so use this time
                    return previousScheduledTime;
                }
                else
                {
                    // Scheduled time has ended up in the past, but not just started
                    return getNextScheduledTime(previousScheduledTime, interval, timePointNow);
                }
            }
        }
        else if (newInstall)
        {
            LOGINFO("This is first time tscheduler is running");
            // run telemetry in ten minutes when first update should have finished
            return timePointNow + minutes{10};
        }
        else
        {
            // Status file is corrupt but was present - now deleted
            // run in one day
            return timePointNow + seconds{interval};
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

        delayThread = std::make_unique<SleepyThread>(delayUntil, std::move(task), m_taskQueue);
        delayThread->start();
    }

    bool SchedulerProcessor::isTelemetryDisabled(
        const system_clock::time_point& previousScheduledTime,
        bool schedulerStatusFileValid,
        unsigned int interval,
        bool configFileValid)
    {
        // Telemetry can be disabled remotely by setting the interval to zero. Telemetry can be disabled locally be
        // setting the scheduled time in the status file to the epoch.

        const time_point epoch;

        return (schedulerStatusFileValid && previousScheduledTime == epoch) || !configFileValid || interval == 0 ||
               m_telemetryHost.empty();
    }

    void SchedulerProcessor::waitToRunTelemetry(bool runScheduledInPastNow)
    {
        // Always re-read values from the telemetry configuration (supplementary) and status files in case they've been
        // externally updated.
        bool newInstall =
            !(Common::FileSystem::fileSystem()->isFile(m_pathManager.getTelemetrySchedulerStatusFilePath()));
        auto const& [schedulerStatus, schedulerStatusFileValid] = getStatusFromFile();
        auto const& [telemetryConfig, configFileValid] = getConfigFromFile();
        auto previousScheduledTime = schedulerStatus.getTelemetryScheduledTime();
        auto interval = telemetryConfig.getInterval();

        if (isTelemetryDisabled(previousScheduledTime, schedulerStatusFileValid, interval, configFileValid))
        {
            LOGINFO(
                "Telemetry reporting is currently disabled - will check again in " << m_configurationCheckDelay.count()
                                                                                   << " seconds");
            delayBeforeQueueingTask(
                system_clock::now() + m_configurationCheckDelay,
                m_delayBeforeCheckingConfiguration,
                { .taskType = SchedulerTask::TaskType::InitialWaitToRunTelemetry });
            return;
        }

        const system_clock::time_point epoch;

        std::optional<SchedulerStatus> status;
        if (schedulerStatusFileValid)
        {
            status = schedulerStatus;
        }
        system_clock::time_point scheduledTime = calculateScheduledTimeFromtheStatusFile(status,
                                                                                          newInstall, interval, runScheduledInPastNow,
                                                 system_clock::now());
        assert(scheduledTime != epoch);
        {
            auto statusCopy = schedulerStatus;
            statusCopy.setTelemetryScheduledTime(scheduledTime);
            saveStatusFile(statusCopy);
        }

        // Do the following whether the time scheduled is in the past or in the future.

        std::string formattedScheduledTime =
            Common::UtilityImpl::TimeUtils::fromTime(system_clock::to_time_t(scheduledTime));
        LOGINFO("Telemetry reporting is scheduled to run at " << formattedScheduledTime);

        delayBeforeQueueingTask(
            scheduledTime, m_delayBeforeRunningTelemetry, { .taskType = SchedulerTask::TaskType::RunTelemetry });
    }

    void SchedulerProcessor::runTelemetry()
    {
        // Always re-read values from the telemetry configuration (supplementary) and status files in case they've been
        // externally updated.

        auto const& [schedulerStatus, schedulerStatusFileValid] = getStatusFromFile();
        auto [telemetryConfig, configFileValid] = getConfigFromFile();
        auto previousScheduledTime = schedulerStatus.getTelemetryScheduledTime();
        auto interval = telemetryConfig.getInterval();

        if (isTelemetryDisabled(previousScheduledTime, schedulerStatusFileValid, interval, configFileValid))
        {
            // Waits until either the scheduled time, or updates the scheduled time to be in the future again and waits
            // until then
            m_taskQueue->push({ .taskType = SchedulerTask::TaskType::WaitToRunTelemetry });
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
                    { .taskType = SchedulerTask::TaskType::InitialWaitToRunTelemetry });
                return;
            }

            machineId.erase(
                std::remove_if(
                    machineId.begin(), machineId.end(), [](auto const& c) -> bool { return !std::isalnum(c); }),
                machineId.end());

            std::string resourceName = machineId + ".json";

            Config telemetryExeConfig = Common::TelemetryConfigImpl::Config::buildExeConfigFromTelemetryConfig(
                telemetryConfig, m_telemetryHost, resourceName);

            Common::FileSystem::fileSystem()->writeFile(
                m_pathManager.getTelemetryExeConfigFilePath(), Serialiser::serialise(telemetryExeConfig));

            // Record we are about to launch telemetry
            auto now = clock::now();
            saveLastStartTimeStatus(now);

            m_telemetryExeProcess = Common::Process::createProcess();

            m_telemetryExeProcess->setOutputLimit(MAX_OUTPUT_SIZE);
            m_telemetryExeProcess->exec(
                m_pathManager.getTelemetryExecutableFilePath(), { m_pathManager.getTelemetryExeConfigFilePath() });

            LOGINFO(
                "Telemetry executable's state will be checked in " << m_telemetryExeCheckDelay.count() << " seconds");
            const auto timeToCheckExeState = system_clock::now() + m_telemetryExeCheckDelay;
            delayBeforeQueueingTask(
                timeToCheckExeState, m_delayBeforeCheckingExe, { SchedulerTask::TaskType::CheckExecutableFinished });
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("File access error writing " << m_pathManager.getTelemetryExeConfigFilePath() << ": " << e.what());
            m_taskQueue->push({ SchedulerTask::TaskType::WaitToRunTelemetry });
        }
        catch (const Common::Process::IProcessException& processException)
        {
            LOGERROR("Running telemetry executable failed: " << processException.what());
            m_taskQueue->push({ SchedulerTask::TaskType::WaitToRunTelemetry });
        }
    }

    void SchedulerProcessor::checkExecutableFinished()
    {
        // Telemetry executable should have been run before this point, hence early exit if
        // m_telemetryExeProcess == nullptr
        if (m_telemetryExeProcess == nullptr)
        {
            return;
        }

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

        m_taskQueue->push({ .taskType = SchedulerTask::TaskType::WaitToRunTelemetry });
    }

    void SchedulerProcessor::processALCPolicy(const std::string& policyXml)
    {
        try
        {
            // Note: If malformed hostname given by ALC Policy then a PolicyParse Exception is thrown
            // No more tasks end up on queue so TelemetryScheduler just waits for another ALC Policy
            Common::Policy::ALCPolicy alcPolicy{ policyXml };
            const auto telemetryHost = alcPolicy.getTelemetryHost();

            if (!telemetryHost) // <Telemetry> field did not exist in ALC policy
            {
                LOGDEBUG("Didn't get telemetry host from ALC policy");
                m_telemetryHost = "t1.sophosupd.com"; // Fallback to hardcoded value
            }
            else if (telemetryHost->empty()) // Value of <Telemetry> field was empty or invalid host found in policy
            {
                LOGDEBUG("Got empty host from ALC policy");
                m_telemetryHost = ""; // Don't do Telemetry
            }
            else
            {
                LOGDEBUG("Got telemetry host '" << *telemetryHost << "' from ALC policy");
                m_telemetryHost = *telemetryHost;
            }

            bool wasAlcPolicyProcessedBefore = m_alcPolicyProcessed;
            m_alcPolicyProcessed = true;

            if (!wasAlcPolicyProcessedBefore)
            {
                m_taskQueue->push(
                    { .taskType = SchedulerTask::TaskType::InitialWaitToRunTelemetry, .content = "", .appId = "" });
            }
        }
        catch (const Common::Policy::PolicyParseException& ex)
        {
            LOGERROR("Failed to parse ALC policy: " << ex.what());
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to write telemetry config to file: " << ex.what());
        }
    }
} // namespace TelemetrySchedulerImpl