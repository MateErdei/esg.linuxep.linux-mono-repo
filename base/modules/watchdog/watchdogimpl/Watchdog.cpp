/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Watchdog.h"

#include "Logger.h"
#include "PluginProxy.h"

#include "Common/ProcessMonitoringImpl/SignalHandler.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <cassert>
#include <cstdlib>
#include <unistd.h>

namespace
{
    const char* PLUGINNOTFOUND = "Error: Plugin not found";
}

using namespace watchdog::watchdogimpl;

Watchdog::Watchdog(Common::ZMQWrapperApi::IContextSharedPtr context) :
    Common::ProcessMonitoringImpl::ProcessMonitor(context),
    m_watchdogservice(context)
{
}
Watchdog::Watchdog() : Watchdog(Common::ZMQWrapperApi::createContext()) {}

Watchdog::~Watchdog()
{
    m_socket.reset();
    m_context.reset();
}

int Watchdog::initialiseAndRun()
{
    PluginInfoVector pluginConfigs = readPluginConfigs();

    for (auto& info : pluginConfigs)
    {
        addProcessToMonitor(std::unique_ptr<PluginProxy>(new PluginProxy(std::move(info))));
    }

    pluginConfigs.clear();

    setupSocket();

    addReplierSocketAndHandleToPoll(m_socket.get(), [this]() { this->handleSocketRequest(); });

    run();

    // Normal shutdown
    m_socket.reset();
    return 0;
}

PluginInfoVector Watchdog::readPluginConfigs()
{
    return Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();
}

std::string Watchdog::getIPCPath()
{
    return Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress();
}

void Watchdog::setupSocket()
{
    m_socket = m_context->getReplier();
    m_socket->listen(getIPCPath());

    // getIPCPath() has a ipc:// prefix, remove it
    std::string ipcFilesPath = getIPCPath().substr(6);
    try
    {
        Common::FileSystem::filePermissions()->chmod(ipcFilesPath, S_IRUSR | S_IWUSR);
        Common::FileSystem::filePermissions()->chown(ipcFilesPath, "root", "root");
    }
    catch (Common::FileSystem::IFileSystemException& error)
    {
        LOGERROR(error.what());
        throw;
    }
}

void Watchdog::handleSocketRequest()
{
    Common::ZeroMQWrapper::IReadable::data_t request = m_socket->read();
    std::string responseStr = handleCommand(request);

    Common::ZeroMQWrapper::IWritable::data_t response{ responseStr };
    m_socket->write(response);
}

std::string Watchdog::disablePlugin(const std::string& pluginName)
{
    LOGINFO("Requesting stop of " << pluginName);
    PluginProxy* proxy = findPlugin(pluginName);
    if (proxy != nullptr)
    {
        proxy->setEnabled(false);
        return watchdogReturnsOk;
    }
    return PLUGINNOTFOUND;
}

std::string Watchdog::enablePlugin(const std::string& pluginName)
{
    LOGINFO("Starting " << pluginName);

    PluginProxy* proxy = findPlugin(pluginName); // BORROWED pointer

    std::pair<Common::PluginRegistryImpl::PluginInfo, bool> loadResult =
        Common::PluginRegistryImpl::PluginInfo::loadPluginInfoFromRegistry(pluginName);

    if (proxy == nullptr && !loadResult.second)
    {
        // No plugin loaded and none on disk
        return PLUGINNOTFOUND;
    }
    if (proxy == nullptr)
    {
        // Not previously loaded, but now available
        assert(loadResult.second);
        addProcessToMonitor(std::unique_ptr<PluginProxy>(new PluginProxy(std::move(loadResult.first))));
        proxy = findPlugin(pluginName); // BORROWED pointer
    }
    else if (loadResult.second)
    {
        // update info from disk
        proxy->updatePluginInfo(loadResult.first);
    }
    proxy->setEnabled(true);
    return watchdogReturnsOk;
}

std::string Watchdog::removePlugin(const std::string& pluginName)
{
    LOGINFO("Removing " << pluginName);

    bool found = false;
    for (auto it = m_processProxies.begin(); it != m_processProxies.end();)
    {
        auto pluginProxy = dynamic_cast<PluginProxy*>(it->get());
        assert(pluginProxy != nullptr);
        if (pluginProxy == nullptr) // necessary for release build.
        {
            break;
        }
        if (pluginName == pluginProxy->name())
        {
            pluginProxy->stop();
            it = m_processProxies.erase(it);
            found = true;
        }
        else
        {
            ++it;
        }
    }

    if (!found)
    {
        // Maybe should just return "OK"?
        return PLUGINNOTFOUND;
    }

    return watchdogReturnsOk;
}

std::string Watchdog::handleCommand(Common::ZeroMQWrapper::IReadable::data_t request)
{
    if (request.size() != 2)
    {
        return "Error: Wrong number of arguments in IPC";
    }
    std::string command = request.at(0);
    std::string argument = request.at(1);
    LOGINFO("Command from IPC: " << command);

    if (command == "STOP")
    {
        return disablePlugin(argument);
    }
    else if (command == "START")
    {
        return enablePlugin(argument);
    }
    else if (command == "REMOVE")
    {
        return removePlugin(argument);
    }
    else if (command == "ISRUNNING")
    {
        return checkPluginIsRunning(argument);
    }

    return "Error: Unknown command";
}

PluginProxy* Watchdog::findPlugin(const std::string& pluginName)
{
    for (auto& proxy : m_processProxies)
    {
        auto pluginProxy = dynamic_cast<PluginProxy*>(proxy.get());
        if (pluginProxy->name() == pluginName)
        {
            return pluginProxy;
        }
    }
    return nullptr;
}

std::string Watchdog::checkPluginIsRunning(const std::string& pluginName)
{
    auto plugin = findPlugin(pluginName);
    if (!plugin)
    {
        return PLUGINNOTFOUND;
    }
    if (plugin->isRunning())
    {
        return watchdogReturnsOk;
    }
    return watdhdogReturnsNotRunning;
}
