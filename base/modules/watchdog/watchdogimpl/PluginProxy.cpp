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

std::chrono::seconds PluginProxy::checkForExit()
{
    auto statusCode = status();
    if (statusCode == Common::Process::ProcessStatus::FINISHED  && m_enabled)
    {
        Common::Telemetry::TelemetryHelper::getInstance().increment(
                watchdog::watchdogimpl::createUnexpectedRestartTelemetryKeyFromPluginName(name()),
                0UL
        );
    }
    return ProcessProxy::checkForExit();

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