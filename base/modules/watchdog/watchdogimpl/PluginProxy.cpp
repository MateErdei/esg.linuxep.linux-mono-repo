/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginProxy.h"

watchdog::watchdogimpl::PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info)
    : m_info(std::move(info)),m_process(Common::Process::createProcess())
{

}

void watchdog::watchdogimpl::PluginProxy::start()
{
    m_process->exec(m_info.getExecutableFullPath(),
        m_info.getExecutableArguments(),
        m_info.getExecutableEnvironmentVariables()
        );
}

void watchdog::watchdogimpl::PluginProxy::stop()
{
    m_process->kill();
}

Common::Process::ProcessStatus watchdog::watchdogimpl::PluginProxy::status()
{
    return m_process->getStatus();
}

int watchdog::watchdogimpl::PluginProxy::exitCode()
{
    return m_process->exitCode();
}
