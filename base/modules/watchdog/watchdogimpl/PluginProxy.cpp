/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginProxy.h"
#include "Logger.h"

watchdog::watchdogimpl::PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info)
    : m_info(std::move(info)),m_process(Common::Process::createProcess())
{
    m_exe = m_info.getExecutableFullPath();
}

void watchdog::watchdogimpl::PluginProxy::start()
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
}

void watchdog::watchdogimpl::PluginProxy::stop()
{
    if (m_exe.empty())
    {
        return;
    }
    LOGINFO("Stopping "<<m_exe);
    m_process->kill();
}

Common::Process::ProcessStatus watchdog::watchdogimpl::PluginProxy::status()
{
    if (m_exe.empty())
    {
        return Common::Process::ProcessStatus::FINISHED;
    }
    return m_process->getStatus();
}

int watchdog::watchdogimpl::PluginProxy::exitCode()
{
    if (m_exe.empty())
    {
        return -1;
    }
    int code = m_process->exitCode();

    LOGINFO(m_exe<<" exited with "<<code);

    return code;
}
