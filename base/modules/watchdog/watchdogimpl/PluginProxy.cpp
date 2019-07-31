/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginProxy.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>

#include <cassert>

using namespace watchdog::watchdogimpl;

PluginProxy::PluginProxy(Common::PluginRegistryImpl::PluginInfo info) :
    ProcessProxy(Common::PluginRegistryImpl::PluginInfoPtr(new Common::PluginRegistryImpl::PluginInfo(std::move(info))))
{
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