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
#include <memory>

using namespace watchdog::watchdogimpl;

PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info) :
    ProcessProxy(std::make_unique<Common::PluginRegistryImpl::PluginInfo>(std::move(info)))
{
}

std::pair<std::chrono::seconds, Common::Process::ProcessStatus> PluginProxy::checkForExit()
{
    bool previousRunning = runningFlag();
    std::pair<std::chrono::seconds, Common::Process::ProcessStatus> processProxyPair = ProcessProxy::checkForExit();
    auto statusCode = processProxyPair.second;
    if (statusCode == Common::Process::ProcessStatus::FINISHED && enabledFlag() && previousRunning)
    {
        auto code = exitCode();
        if (code != RESTART_EXIT_CODE)
        {
            LOGSUPPORT("Update Telemetry Unexpected restart: " << name());
            Common::Telemetry::TelemetryHelper::getInstance().increment(
                watchdog::watchdogimpl::createUnexpectedRestartTelemetryKeyFromPluginName(name()), 1UL);
            auto nativeCode = nativeExitCode();
            Common::Telemetry::TelemetryHelper::getInstance().increment(
                watchdog::watchdogimpl::createUnexpectedRestartTelemetryKeyFromPluginNameAndCode(name(), nativeCode), 1UL);
        }
    }
    return processProxyPair;
}

std::string PluginProxy::name() const
{
    return getPluginInfo().getPluginName();
}

bool PluginProxy::updatePluginInfo(const Common::PluginRegistryImpl::PluginInfo& info)
{
    bool changed = false;
    if (info.getExecutableUserAndGroupAsString() != getPluginInfo().getExecutableUserAndGroupAsString())
    {
        LOGINFO("Executable user and group has changed");
        changed = true;
    }
    if (info.getExecutableFullPath() != getPluginInfo().getExecutableFullPath())
    {
        LOGINFO("Executable path has changed: " << info.getExecutableFullPath());
        changed = true;
    }
    getPluginInfo().copyFrom(info);
    return changed;
}

Common::PluginRegistryImpl::PluginInfo& PluginProxy::getPluginInfo() const
{
    return *dynamic_cast<Common::PluginRegistryImpl::PluginInfo*>(m_processInfo.get());
}