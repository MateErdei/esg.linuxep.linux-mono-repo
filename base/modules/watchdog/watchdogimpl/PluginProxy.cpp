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
    ProcessProxy(Common::PluginRegistryImpl::PluginInfoPtr(new Common::PluginRegistryImpl::PluginInfo(std::move(info)))) {}

std::string PluginProxy::name() const
{
    return getPluginInfo().getPluginName();
}

void PluginProxy::updatePluginInfo(const Common::PluginRegistryImpl::PluginInfo& info)
{
    getPluginInfo().copyFrom(info);
}

//PluginProxy& PluginProxy::operator=(PluginProxy&& other) noexcept
//{
//    if (&other == this)
//    {
//        return *this;
//    }
//
//    swap(other);
//
//    return *this;
//}
//
//void PluginProxy::swap(PluginProxy& other)
//{
//    if (&other == this)
//    {
//        return;
//    }
//
//    std::swap(m_info, other.m_info);
//    std::swap(m_exe, other.m_exe);
//    std::swap(m_running, other.m_running);
//    std::swap(m_deathTime, other.m_deathTime);
//    std::swap(m_enabled, other.m_enabled);
//    std::swap(m_process, other.m_process);
//}
//
//PluginProxy::PluginProxy(PluginProxy&& other) noexcept
//{
//    swap(other);
//}
//

Common::PluginRegistryImpl::PluginInfo& PluginProxy::getPluginInfo() const
{
    return *dynamic_cast<Common::PluginRegistryImpl::PluginInfo*>(m_processInfo.get());
}