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
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <json.hpp>
#include <thread>

namespace TelemetrySchedulerImpl
{
    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<TaskQueue> taskQueue,
        const Common::ApplicationConfiguration::IApplicationPathManager& pathManager) :
        m_taskQueue(std::move(taskQueue)),
        m_pathManager(pathManager),
        m_delayBeforeCheckingConfigurationState(false),
        m_delayBeforeRunningTelemetryState(false)
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

    void SchedulerProcessor::delayBeforeQueueingTask(size_t delayInSeconds, std::atomic<bool>& runningState, Task task)
    {
        if (runningState)
        {
            return; // already running, so don't run another of these delay threads
        }

        runningState = true;

        std::thread delayThread([&] {
          std::this_thread::sleep_for(std::chrono::seconds(delayInSeconds));
          m_taskQueue->push(task);
          runningState = false;
        });

        delayThread.detach();
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
                    Common::ApplicationConfiguration::applicationPathManager().getTelemetrySchedulerStatusFilePath(),
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
            const size_t recheckConfigurationDelayInSeconds = 3600UL; // TODO: breakout as configuration option?
            LOGINFO(
                "Telemetry reporting is currently disabled - will check again in " << recheckConfigurationDelayInSeconds
                                                                                   << " seconds");
            delayBeforeQueueingTask(
                recheckConfigurationDelayInSeconds, m_delayBeforeCheckingConfigurationState, Task::WaitToRunTelemetry);
            return;
        }

        auto now = std::chrono::system_clock::now();
        size_t nowInSecondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        if (scheduledTimeInSecondsSinceEpoch > nowInSecondsSinceEpoch)
        {
            size_t delay = scheduledTimeInSecondsSinceEpoch - nowInSecondsSinceEpoch;
            LOGINFO("Telemetry reporting is scheduled to in " << delay << " seconds");
            delayBeforeQueueingTask(
                delay,
                m_delayBeforeRunningTelemetryState,
                Task::RunTelemetry);
        }
        else
        {
            LOGINFO("Telemetry reporting is scheduled to run now");
            m_taskQueue->push(Task::RunTelemetry);
        }
    }

    void SchedulerProcessor::runTelemetry()
    {
        std::string supplementaryConfigJson = Common::FileSystem::fileSystem()->readFile(
            m_pathManager.getTelemetrySupplementaryFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE); // error checking here

        Common::TelemetryExeConfigImpl::Config config;

        Common::FileSystem::fileSystem()->writeFile(
            m_pathManager.getTelemetryExeConfigFilePath(),
            Common::TelemetryExeConfigImpl::Serialiser::serialise(
                Common::TelemetryExeConfigImpl::Serialiser::deserialise(supplementaryConfigJson)));

        // TODO: LINUXEP-7984 run telemetry executable
        auto processPtr = Common::Process::createProcess();
        int exitCode = 0;
        unsigned int retries = 0;

        try
        {
            processPtr->setOutputLimit(Common::TelemetryExeConfigImpl::ONE_KBYTE_SIZE);
            processPtr->exec(
                m_pathManager.getTelemetryExeConfigFilePath(), { m_pathManager.getTelemetryExeConfigFilePath() });
            while (!m_taskQueue->stopReceived() &&
                   retries < Common::TelemetryExeConfigImpl::DEFAULT_PROCESS_WAIT_RETRIES)
            {
                if (processPtr->wait(
                        Common::Process::milli(Common::TelemetryExeConfigImpl::DEFAULT_PROCESS_WAIT_TIME),
                        Common::TelemetryExeConfigImpl::DEFAULT_PROCESS_WAIT_RETRIES) !=
                    Common::Process::ProcessStatus::FINISHED)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(30));
                    ++retries;
                    continue;
                }

                if (processPtr->getStatus() != Common::Process::ProcessStatus::FINISHED)
                {
                    processPtr->kill();
                    throw Common::Process::IProcessException("Process execution timed out");
                }

                exitCode = processPtr->exitCode();
                if (exitCode != 0)
                {
                    throw Common::Process::IProcessException(
                        "Process returned non-zero exit code, 'Exit code: " + std::string(strerror(exitCode)) + "'");
                }
            }
        }
        catch (const Common::Process::IProcessException& processException)
        {
            LOGERROR("Failed to send telemetry. Reason: " << processException.what());
        }
    }
} // namespace TelemetrySchedulerImpl
