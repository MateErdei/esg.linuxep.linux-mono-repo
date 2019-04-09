/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginProxy.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>

#include <cassert>
#include <Common/Exceptions/Print.h>

using namespace watchdog::watchdogimpl;

PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info) :
    m_info(std::move(info)),
    m_process(Common::Process::createProcess()),
    m_running(false),
    m_deathTime(0),
    m_enabled(true)
{
    m_exe = m_info.getExecutableFullPath();
    if ((!m_exe.empty()) && m_exe[0] != '/')
    {
        // Convert relative path to absolute path relative to installation directory
        std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        m_exe = Common::FileSystem::join(INST, m_exe);
    }
    assert(m_process != nullptr);
    m_process->setOutputLimit(1024 * 1024);
}

void PluginProxy::start()
{
    std::pair<bool, uid_t> userId = m_info.getExecutableUser();
    std::pair<bool, gid_t> groupId = m_info.getExecutableGroup();

    if (m_exe.empty())
    {
        LOGINFO("Not starting plugin without executable");
        return;
    }

    if (!userId.first || !groupId.first)
    {
        LOGERROR("Not starting plugin: invalid user name or group name");
        return;
    }

    LOGINFO("Starting " << m_exe);
    assert(m_process != nullptr);

    // Add in the installation directory to the environment variables used when starting all plugins
    Common::PluginRegistryImpl::PluginInfo::EnvPairs envVariables = m_info.getExecutableEnvironmentVariables();
    envVariables.emplace_back(
        "SOPHOS_INSTALL", Common::ApplicationConfiguration::applicationPathManager().sophosInstall());

    m_process->exec(m_exe, m_info.getExecutableArguments(), envVariables, userId.second, groupId.second);
    m_running = true;
}

void PluginProxy::stop()
{
    if (m_exe.empty())
    {
        return;
    }
    if (m_running)
    {
        LOGINFO("Stopping " << m_exe);
        assert(m_process != nullptr);
        m_process->kill();
    }
}

Common::Process::ProcessStatus PluginProxy::status()
{
    if (m_exe.empty())
    {
        return Common::Process::ProcessStatus::FINISHED;
    }
    assert(m_process != nullptr);
    return m_process->getStatus();
}

int PluginProxy::exitCode()
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

std::chrono::seconds PluginProxy::checkForExit()
{
    if (!m_running)
    {
        // Don't print out multiple times
        return std::chrono::hours(1);
    }
    auto statusCode = status();
    if (statusCode == Common::Process::ProcessStatus::FINISHED)
    {
        int code = exitCode();
        if (code != 0)
        {
            // Always log if the process exited with a non-zero code
            LOGERROR(m_exe << " died with " << code);
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
        return std::chrono::seconds(5);
    }
    return std::chrono::hours(1);
}

std::chrono::seconds PluginProxy::ensureStateMatchesOptions()
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

PluginProxy::~PluginProxy() noexcept
{
    try
    {
        stop();
    }
    catch(...)
    {
        PRINT("Exception while attempting to stop PluginProxy in destructor");
    }
}

std::string PluginProxy::name() const
{
    return m_info.getPluginName();
}

void PluginProxy::setEnabled(bool enabled)
{
    m_enabled = enabled;
    m_deathTime = 0; // If enabled we want to start as soon as possible
}

void PluginProxy::updatePluginInfo(const Common::PluginRegistryImpl::PluginInfo& info)
{
    m_info.copyFrom(info);
}

PluginProxy& PluginProxy::operator=(PluginProxy&& other) noexcept
{
    if (&other == this)
    {
        return *this;
    }

    swap(other);

    return *this;
}

void PluginProxy::swap(PluginProxy& other)
{
    if (&other == this)
    {
        return;
    }

    std::swap(m_info, other.m_info);
    std::swap(m_exe, other.m_exe);
    std::swap(m_running, other.m_running);
    std::swap(m_deathTime, other.m_deathTime);
    std::swap(m_enabled, other.m_enabled);
    std::swap(m_process, other.m_process);
}

PluginProxy::PluginProxy(PluginProxy&& other) noexcept
{
    swap(other);
}
