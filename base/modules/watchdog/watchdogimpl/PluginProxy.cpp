/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginProxy.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <watchdog/watchdogimpl/Watchdog.h>

#include <cassert>

using namespace watchdog::watchdogimpl;

PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info) :
    ProcessProxy(Common::PluginRegistryImpl::PluginInfoPtr(new Common::PluginRegistryImpl::PluginInfo(std::move(info))))
{
}

std::pair<std::chrono::seconds, Common::Process::ProcessStatus> PluginProxy::checkForExit()
{
    bool previousRunning = runningFlag();
    std::pair<std::chrono::seconds, Common::Process::ProcessStatus> processProxyPair = ProcessProxy::checkForExit();
    auto statusCode = processProxyPair.second;
    if (statusCode == Common::Process::ProcessStatus::FINISHED && enabledFlag() && previousRunning)
    {
        LOGSUPPORT("Update Telemetry Unexpected restart: " << name());
        Common::Telemetry::TelemetryHelper::getInstance().increment(
            watchdog::watchdogimpl::createUnexpectedRestartTelemetryKeyFromPluginName(name()), 1UL);
    }
    return processProxyPair;
}

std::string PluginProxy::name() const
{
    return getPluginInfo().getPluginName();
}

void PluginProxy::updatePluginInfo(const Common::PluginRegistryImpl::PluginInfo& info)
{
    getPluginInfo().copyFrom(info);
}

Common::PluginRegistryImpl::PluginInfo& PluginProxy::getPluginInfo() const
{
    return *dynamic_cast<Common::PluginRegistryImpl::PluginInfo*>(m_processInfo.get());
}