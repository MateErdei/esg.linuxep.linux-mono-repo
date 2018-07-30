/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginProxy.h"
#include "Logger.h"

using namespace watchdog::watchdogimpl;

PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info)
    : m_info(std::move(info)),m_process(Common::Process::createProcess()),m_running(false),m_deathTime(0)
{
    m_exe = m_info.getExecutableFullPath();
    m_process->setOutputLimit(1024*1024);
}

void PluginProxy::start()
{
    if (m_exe.empty())
    {
        LOGINFO("Not starting plugin without executable");
        return;
    }
    LOGINFO("Starting "<<m_exe);
    m_process->exec(m_exe,
        m_info.getExecutableArguments(),
        m_info.getExecutableEnvironmentVariables()
        );
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
        m_process->kill();
    }
}

Common::Process::ProcessStatus PluginProxy::status()
{
    if (m_exe.empty())
    {
        return Common::Process::ProcessStatus::FINISHED;
    }
    return m_process->getStatus();
}

int PluginProxy::exitCode()
{
    if (m_exe.empty())
    {
        return -1;
    }
    int code = m_process->exitCode();

    std::string output = m_process->output();
    LOGINFO("Output: "<<output);

    return code;
}

void PluginProxy::checkForExit()
{
    if (!m_running)
    {
        // Don't print out multiple times
        return;
    }
    if (status() == Common::Process::ProcessStatus::FINISHED)
    {
        int code = exitCode();
        if (code != 0)
        {
            LOGERROR("Process died with " << code);
        }
        m_running = false;
        m_deathTime = ::time(nullptr);
    }
}

time_t PluginProxy::startIfRequired()
{
    if (m_running)
    {
        return 3600;
    }

    time_t now = ::time(nullptr);
    if ((now - m_deathTime) > 10)
    {
        start();
        return 3600;
    }
    else
    {
        LOGDEBUG("Not starting "<<m_exe);
        return 10 - (now - m_deathTime);
    }
}

PluginProxy::~PluginProxy() noexcept
{
    stop();
}
