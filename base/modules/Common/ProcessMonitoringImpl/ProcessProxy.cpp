// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "ProcessProxy.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Exceptions/Print.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <cassert>
#include <cmath>

namespace Common::ProcessMonitoringImpl
{
    ProcessProxy::ProcessSharedState::ProcessSharedState() :
        m_mutex{},
        m_enabled(true),
        m_running(false),
        m_process(Common::Process::createProcess()),
        m_deathTime(0),
        m_killIssuedTime(0)
    {
    }

    ProcessProxy::ProcessProxy(Common::Process::IProcessInfoPtr processInfo) :
        m_processInfo(std::move(processInfo)), m_sharedState()
    {
        resetBackoff();
        m_exe = m_processInfo->getExecutableFullPath();
        if ((!m_exe.empty()) && m_exe[0] != '/')
        {
            // Convert relative path to absolute path relative to installation directory
            std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
            m_exe = Common::FileSystem::join(INST, m_exe);
        }
        assert(m_sharedState.m_process != nullptr);
        m_sharedState.m_process->setOutputLimit(1024 * 1024);
        m_sharedState.m_process->setNotifyProcessFinishedCallBack(
            [this]()
            {
                LOGDEBUG("Notify process finished callback called");
                m_termination_callback_called.store(true);
                notifyProcessTerminationCallbackPipe(); // Will definitely happen after the store
            });
    }

    void ProcessProxy::start()
    {
        m_processInfo->refreshUserAndGroupIds();
        std::pair<bool, uid_t> userId = m_processInfo->getExecutableUser();
        std::pair<bool, gid_t> groupId = m_processInfo->getExecutableGroup();
        std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        m_exe = Common::FileSystem::join(INST, m_processInfo->getExecutableFullPath());

        if (!FileSystem::fileSystem()->isFile(m_exe))
        {
            LOGINFO("Executable does not exist at : " << m_exe);
            return;
        }

        if (!userId.first || !groupId.first)
        {
            LOGERROR("Not starting plugin: invalid user name or group name");
            return;
        }

        LOGINFO("Starting " << m_exe);
        assert(m_sharedState.m_process != nullptr);
        m_sharedState.m_killIssuedTime = 0;
        m_termination_callback_called = false;
        // Add in the installation directory to the environment variables used when starting all plugins
        Common::Process::EnvPairs envVariables = m_processInfo->getExecutableEnvironmentVariables();
        envVariables.emplace_back(
            "SOPHOS_INSTALL", Common::ApplicationConfiguration::applicationPathManager().sophosInstall());

        m_sharedState.m_process->exec(
            m_exe, m_processInfo->getExecutableArguments(), envVariables, userId.second, groupId.second);
        m_sharedState.m_running = true;
    }

    void ProcessProxy::stop()
    {
        std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
        lock_acquired_stop();
    }
    void ProcessProxy::lock_acquired_stop()
    {
        if (m_exe.empty())
        {
            return;
        }

        if (m_sharedState.m_running)
        {
            m_sharedState.m_killIssuedTime = Common::UtilityImpl::TimeUtils::getCurrTime();
            LOGINFO("Stopping " << m_exe);
            assert(m_sharedState.m_process != nullptr);
            m_sharedState.m_process->kill(m_processInfo->getSecondsToShutDown());
        }
    }

    Common::Process::ProcessStatus ProcessProxy::status()
    {
        if (m_exe.empty())
        {
            return Common::Process::ProcessStatus::FINISHED;
        }
        assert(m_sharedState.m_process != nullptr);
        return m_sharedState.m_process->getStatus();
    }

    int ProcessProxy::exitCode()
    {
        if (m_exe.empty())
        {
            return -1;
        }
        assert(m_sharedState.m_process != nullptr);
        int code = m_sharedState.m_process->exitCode();

        std::string output = m_sharedState.m_process->output();
        if (!output.empty())
        {
            // Don't log if process had no output
            LOGINFO("Output: " << output);
        }

        return code;
    }

    int ProcessProxy::nativeExitCode()
    {
        if (m_exe.empty())
        {
            return -1;
        }
        assert(m_sharedState.m_process != nullptr);
        int code = m_sharedState.m_process->nativeExitCode();

        return code;
    }

    ProcessProxy::exit_status_t ProcessProxy::checkForExit()
    {
        std::unique_lock<std::mutex> lock(m_sharedState.m_mutex);
        Process::ProcessStatus statusCode;
        if (m_termination_callback_called)
        {
            m_termination_callback_called = false;

            LOGINFO("Waiting for " << m_exe << " to exit");
            auto start = std::chrono::steady_clock::now();
            // In the process of terminating, so need to wait for exit
            using Common::Process::Milliseconds;
            lock.unlock(); // Lock has to be released while we wait, since callback could take it
            statusCode = m_sharedState.m_process->wait(Milliseconds{ 10 }, 10);
            lock.lock();
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            LOGDEBUG("Finished waiting for " << m_exe << " to exit after " << duration.count() << "ms");
        }
        else
        {
            statusCode = status();
        }

        if (!m_sharedState.m_running)
        {
            // Don't print out multiple times
            return { std::chrono::hours(1), statusCode };
        }
        if (statusCode == Common::Process::ProcessStatus::FINISHED)
        {
            int code = exitCode();
            m_sharedState.m_lastExit = code;
            if (code != 0)
            {
                if (code == ECANCELED)
                {
                    if (m_sharedState.m_killIssuedTime != 0)
                    {
                        time_t secondsElapsed =
                            Common::UtilityImpl::TimeUtils::getCurrTime() - m_sharedState.m_killIssuedTime;
                        LOGWARN(
                            m_exe << " killed after waiting for " << secondsElapsed
                                  << " seconds for it to stop cleanly");
                    }
                    else
                    {
                        LOGERROR(m_exe << " was forcefully stopped. Code=ECANCELED.");
                    }
                }
                else if (code == RESTART_EXIT_CODE)
                {
                    LOGINFO(m_exe << " exited with " << code << " to be restarted");
                }
                else
                {
                    int native = nativeExitCode();
                    // Always log if the process exited with a non-zero code
                    if (WIFEXITED(native))
                    {
                        // Exited with code
                        int exitCode = WEXITSTATUS(native);
                        LOGERROR(m_exe << " died with exit code " << exitCode);
                    }
                    else if (WIFSIGNALED(native))
                    {
                        // Exited with signal
                        int signal = WTERMSIG(native);
                        if (m_sharedState.m_enabled)
                        {
                            // Error if the process is enabled
                            LOGERROR(m_exe << " died with signal " << signal);
                        }
                        else
                        {
                            LOGINFO(m_exe << " died with signal " << signal << " after being disabled");
                        }
                    }
                }
            }
            else if (m_sharedState.m_enabled)
            {
                // Log if we weren't expecting the process to exit, even with a zero code
                // this can happen when the plugin stops itself eg when edr changes flags
                LOGINFO(m_exe << " exited");
                // Process will be respawned automatically after a delay
            }

            m_sharedState.m_running = false;
            m_sharedState.m_deathTime = ::time(nullptr);
        }
        else if (!m_sharedState.m_enabled)
        {
            LOGWARN(m_exe << " still running, despite being disabled: " << (int)statusCode);
            return { std::chrono::seconds(5), statusCode };
        }
        return { std::chrono::hours(1), statusCode };
    }

    std::chrono::seconds ProcessProxy::ensureStateMatchesOptions()
    {
        std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
        if (!m_sharedState.m_enabled)
        {
            if (m_sharedState.m_running)
            {
                // Running and we don't want it - stop it
                lock_acquired_stop();
            }
            // We don't want it running - wait a long time before calling again
            return m_maximumBackoff; // 1 hour
        }
        else if (m_sharedState.m_running)
        {
            // Running and we want it running, so wait a long time before calling again
            resetBackoff();
            return m_maximumBackoff; // 1 hour
        }

        // Restart immediately if last exit was with RESTART_EXIT_CODE
        auto currentBackoff = getCurrentBackoff();
        time_t now = ::time(nullptr);
        if (m_sharedState.m_lastExit == RESTART_EXIT_CODE || (now - m_sharedState.m_deathTime) >= currentBackoff.count())
        {
            start();
            increaseBackoff();
            return currentBackoff;
        }
        else
        {
            LOGDEBUG("Not starting " << m_exe);
            return std::chrono::seconds(currentBackoff.count() - (now - m_sharedState.m_deathTime));
        }
    }

    void ProcessProxy::setCoreDumpMode(const bool mode)
    {
        m_sharedState.m_process->setCoreDumpMode(mode);
    }

    ProcessProxy::~ProcessProxy() noexcept
    {
        stop();
    }

    void ProcessProxy::shutDownProcessCheckForExit()
    {
        try
        {
            auto start = std::chrono::steady_clock::now();
            stop();
            ProcessProxy::checkForExit();
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            LOGDEBUG(m_exe << " shutdown duration: " << duration.count() << "ms");
        }
        catch (const std::exception& ex)
        {
            PRINT("Exception caught while attempting to stop ProcessProxy in destructor: " << ex.what());
        }
        catch (...)
        {
            PRINT("Non std::exception caught while attempting to stop ProcessProxy in destructor");
        }
    }

    void ProcessProxy::setEnabled(bool enabled)
    {
        std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
        m_sharedState.m_enabled = enabled;
        m_sharedState.m_deathTime = 0; // If enabled we want to start as soon as possible
    }

    bool ProcessProxy::isRunning()
    {
        std::pair<std::chrono::seconds, Process::ProcessStatus> data = checkForExit();
        return data.second == Process::ProcessStatus::RUNNING;
    }

    std::string ProcessProxy::name() const
    {
        return "";
    }

    bool ProcessProxy::runningFlag()
    {
        std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
        return m_sharedState.m_running;
    }

    bool ProcessProxy::enabledFlag()
    {
        std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
        return m_sharedState.m_enabled;
    }
    void ProcessProxy::notifyProcessTerminationCallbackPipe()
    {
        if (m_processTerminationCallbackPipe)
        {
            m_processTerminationCallbackPipe->notify();
        }
    }

    void ProcessProxy::resetBackoff()
    {
        m_unsuccessfulConsecutiveRestarts = 0;
    }

    void ProcessProxy::increaseBackoff()
    {
        // Don't need to worry about buffer overflow as it would require watchdog trying to restart a broken plugin
        // ~2 billion times (which would take more than 2 billion hours as backoff would reach the 1-hour limit)
        // Even if an overflow did happen the variable would just reset back to 0 which won't cause a crash
        m_unsuccessfulConsecutiveRestarts++;
    }

    /**
     * Idea was to have backoff be a geometric series that doubles backoff each unsuccessful and consecutive restart
     * n = m_unsuccessfulConsecutiveRestarts (default is 0)
     * a = m_minimumBackoff (10)
     * r = 2
     * backoff of given n = a * r ^ n
     * backoff caps off at m_maximumBackoff (3600)
     */
    std::chrono::seconds ProcessProxy::getCurrentBackoff()
    {
        auto calculatedBackoff = static_cast<std::chrono::duration<double>>(m_minimumBackoff)
                * std::pow(2.0, m_unsuccessfulConsecutiveRestarts);
        return std::min(std::chrono::duration_cast<std::chrono::seconds>(calculatedBackoff), m_maximumBackoff);
    }

} // namespace Common::ProcessMonitoringImpl

namespace Common::ProcessMonitoring
{
    IProcessProxyPtr createProcessProxy(Common::Process::IProcessInfoPtr processInfoPtr)
    {
        return IProcessProxyPtr(new Common::ProcessMonitoringImpl::ProcessProxy(std::move(processInfoPtr)));
    }
} // namespace Common::ProcessMonitoring