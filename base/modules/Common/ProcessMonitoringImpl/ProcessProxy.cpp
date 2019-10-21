/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessProxy.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Exceptions/Print.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include <cassert>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        ProcessProxy::ProcessSharedState::ProcessSharedState()
        :            m_mutex{},
                     m_enabled(true),
                     m_running(false),
                     m_process(Common::Process::createProcess()),
                     m_deathTime(0),
                     m_killIssuedTime(0)
        {

        }

        ProcessProxy::ProcessProxy(Common::Process::IProcessInfoPtr processInfo) :
            m_processInfo(std::move(processInfo)),
            m_sharedState()
        {
            m_exe = m_processInfo->getExecutableFullPath();
            if ((!m_exe.empty()) && m_exe[0] != '/')
            {
                // Convert relative path to absolute path relative to installation directory
                std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
                m_exe = Common::FileSystem::join(INST, m_exe);
            }
            assert(m_sharedState.m_process != nullptr);
            m_sharedState.m_process->setOutputLimit(1024 * 1024);
        }

        void ProcessProxy::start()
        {
            std::pair<bool, uid_t> userId = m_processInfo->getExecutableUser();
            std::pair<bool, gid_t> groupId = m_processInfo->getExecutableGroup();

            if (m_exe.empty())
            {
                LOGINFO("Not starting plugin without executable");
                return;
            }

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
            // Add in the installation directory to the environment variables used when starting all plugins
            Common::Process::EnvPairs envVariables = m_processInfo->getExecutableEnvironmentVariables();
            envVariables.emplace_back(
                "SOPHOS_INSTALL", Common::ApplicationConfiguration::applicationPathManager().sophosInstall());

            m_sharedState.m_process->exec(
                m_exe, m_processInfo->getExecutableArguments(), envVariables, userId.second, groupId.second);
            m_sharedState.m_running = true;
        }

        void ProcessProxy::stop() {
            std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
            lock_acquired_stop();
        }
        void ProcessProxy::lock_acquired_stop()
        {
            if (m_exe.empty()) {
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
            LOGINFO("Output: " << output);

            return code;
        }

        std::pair<std::chrono::seconds, Process::ProcessStatus> ProcessProxy::checkForExit()
        {
            //
            std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
            auto statusCode = status();
            if (!m_sharedState.m_running)
            {
                // Don't print out multiple times
                return std::pair<std::chrono::seconds, Process::ProcessStatus>(std::chrono::hours(1), statusCode);
            }
            if (statusCode == Common::Process::ProcessStatus::FINISHED)
            {
                int code = exitCode();
                if (code != 0)
                {
                    if (code == ECANCELED)
                    {
                        if (m_sharedState.m_killIssuedTime != 0)
                        {
                            time_t secondsElapsed = Common::UtilityImpl::TimeUtils::getCurrTime() - m_sharedState.m_killIssuedTime;
                            LOGWARN(
                                m_exe << " killed after waiting for " << secondsElapsed
                                      << " seconds for it to stop cleanly");
                        }
                        else
                        {
                            LOGERROR(m_exe << " was forcefully stopped. Code=ECANCELED.");
                        }
                    }
                    else
                    {
                        // Always log if the process exited with a non-zero code
                        LOGERROR(m_exe << " died with " << code);
                    }
                }
                else if (m_sharedState.m_enabled)
                {
                    // Log if we weren't expecting the process to exit, even with a zero code
                    LOGERROR(m_exe << " exited when not expected");
                    // Process will be respawned automatically after a delay
                }

                m_sharedState.m_running = false;
                m_sharedState.m_deathTime = ::time(nullptr);
            }
            else if (!m_sharedState.m_enabled)
            {
                LOGWARN(m_exe << " still running, despite being disabled: " << (int)statusCode);
                return std::pair<std::chrono::seconds, Process::ProcessStatus>(std::chrono::seconds(5), statusCode);
            }
            return std::pair<std::chrono::seconds, Process::ProcessStatus>(std::chrono::hours(1), statusCode);
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
                return std::chrono::hours(1);
            }
            else if (m_sharedState.m_running)
            {
                // Running and we want it running, so wait a long time before calling again
                return std::chrono::hours(1);
            }

            time_t now = ::time(nullptr);
            if ((now - m_sharedState.m_deathTime) > 10)
            {
                start();
                return std::chrono::hours(1);
            }
            else
            {
                LOGDEBUG("Not starting " << m_exe);
                return std::chrono::seconds(10 - (now - m_sharedState.m_deathTime));
            }
        }

        ProcessProxy::~ProcessProxy() noexcept
        {
            try
            {
                stop();
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

            std::pair<std::chrono::seconds, Process::ProcessStatus>  data = checkForExit();
            return data.second == Process::ProcessStatus::RUNNING;
        }

        std::string ProcessProxy::name() const
        {
            return "";
        }

        bool ProcessProxy::runningFlag() {
            std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
            return m_sharedState.m_running;
        }

        bool ProcessProxy::enabledFlag() {
            std::lock_guard<std::mutex> lock(m_sharedState.m_mutex);
            return m_sharedState.m_enabled;
        }

    } // namespace ProcessMonitoringImpl
} // namespace Common

namespace Common::ProcessMonitoring
{
    IProcessProxyPtr createProcessProxy(Common::Process::IProcessInfoPtr processInfoPtr)
    {
        return IProcessProxyPtr(new Common::ProcessMonitoringImpl::ProcessProxy(std::move(processInfoPtr)));
    }
} // namespace Common::ProcessMonitoring