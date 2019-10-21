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
        ProcessProxy::ProcessProxy(Common::Process::IProcessInfoPtr processInfo) :
            m_processInfo(std::move(processInfo)),
            m_enabled(true),
            m_running(false),
            m_process(Common::Process::createProcess()),
            m_deathTime(0),
            m_killIssuedTime(0)
        {
            m_exe = m_processInfo->getExecutableFullPath();
            if ((!m_exe.empty()) && m_exe[0] != '/')
            {
                // Convert relative path to absolute path relative to installation directory
                std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
                m_exe = Common::FileSystem::join(INST, m_exe);
            }
            assert(m_process != nullptr);
            m_process->setOutputLimit(1024 * 1024);
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
            assert(m_process != nullptr);
            m_killIssuedTime = 0;
            // Add in the installation directory to the environment variables used when starting all plugins
            Common::Process::EnvPairs envVariables = m_processInfo->getExecutableEnvironmentVariables();
            envVariables.emplace_back(
                "SOPHOS_INSTALL", Common::ApplicationConfiguration::applicationPathManager().sophosInstall());

            m_process->exec(
                m_exe, m_processInfo->getExecutableArguments(), envVariables, userId.second, groupId.second);
            m_running = true;
        }

        void ProcessProxy::stop()
        {
            if (m_exe.empty())
            {
                return;
            }
            if (m_running)
            {
                m_killIssuedTime = Common::UtilityImpl::TimeUtils::getCurrTime();
                LOGINFO("Stopping " << m_exe);
                assert(m_process != nullptr);
                m_process->kill(m_processInfo->getSecondsToShutDown());
            }
        }

        Common::Process::ProcessStatus ProcessProxy::status()
        {
            if (m_exe.empty())
            {
                return Common::Process::ProcessStatus::FINISHED;
            }
            assert(m_process != nullptr);
            return m_process->getStatus();
        }

        int ProcessProxy::exitCode()
        {
            if (m_exe.empty())
            {
                return -1;
            }
            assert(m_process != nullptr);
            int code = m_process->exitCode();

            std::string output = m_process->output();
            LOGINFO("Output: " << output);

            return code;
        }

        std::pair<std::chrono::seconds, Process::ProcessStatus> ProcessProxy::checkForExit()
        {
            auto statusCode = status();
            if (!m_running)
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
                        if (m_killIssuedTime != 0)
                        {
                            time_t secondsElapsed = Common::UtilityImpl::TimeUtils::getCurrTime() - m_killIssuedTime;
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
                else if (m_enabled)
                {
                    // Log if we weren't expecting the process to exit, even with a zero code
                    LOGERROR(m_exe << " exited when not expected");
                    // Process will be respawned automatically after a delay
                }

                m_running = false;
                m_deathTime = ::time(nullptr);
            }
            else if (!m_enabled)
            {
                LOGWARN(m_exe << " still running, despite being disabled: " << (int)statusCode);
                return std::pair<std::chrono::seconds, Process::ProcessStatus>(std::chrono::seconds(5), statusCode);
            }
            return std::pair<std::chrono::seconds, Process::ProcessStatus>(std::chrono::hours(1), statusCode);
        }

        std::chrono::seconds ProcessProxy::ensureStateMatchesOptions()
        {
            if (!m_enabled)
            {
                if (m_running)
                {
                    // Running and we don't want it - stop it
                    stop();
                }
                // We don't want it running - wait a long time before calling again
                return std::chrono::hours(1);
            }
            else if (m_running)
            {
                // Running and we want it running, so wait a long time before calling again
                return std::chrono::hours(1);
            }

            time_t now = ::time(nullptr);
            if ((now - m_deathTime) > 10)
            {
                start();
                return std::chrono::hours(1);
            }
            else
            {
                LOGDEBUG("Not starting " << m_exe);
                return std::chrono::seconds(10 - (now - m_deathTime));
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
            m_enabled = enabled;
            m_deathTime = 0; // If enabled we want to start as soon as possible
        }

        ProcessProxy& ProcessProxy::operator=(ProcessProxy&& other) noexcept
        {
            if (&other == this)
            {
                return *this;
            }

            swap(other);

            return *this;
        }

        void ProcessProxy::swap(ProcessProxy& other)
        {
            if (&other == this)
            {
                return;
            }

            std::swap(m_processInfo, other.m_processInfo);
            std::swap(m_exe, other.m_exe);
            std::swap(m_running, other.m_running);
            std::swap(m_deathTime, other.m_deathTime);
            std::swap(m_enabled, other.m_enabled);
            std::swap(m_process, other.m_process);
            std::swap(m_killIssuedTime, other.m_killIssuedTime);
        }

        ProcessProxy::ProcessProxy(ProcessProxy&& other) noexcept { swap(other); }

        bool ProcessProxy::isRunning()
        {
            checkForExit();
            return m_running;
        }

        std::string ProcessProxy::name() const
        {
            return "";
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