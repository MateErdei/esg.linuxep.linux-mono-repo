/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Watchdog.h"

#include "Logger.h"
#include "PluginProxy.h"
#include "SignalHandler.h"

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

using namespace watchdog::watchdogimpl;

Watchdog::Watchdog() : Watchdog(Common::ZMQWrapperApi::createContext()) {}

Watchdog::Watchdog(Common::ZMQWrapperApi::IContextSharedPtr context) : m_context(std::move(context)) {}

Watchdog::~Watchdog()
{
    m_socket.reset();
    m_context.reset();
}

int Watchdog::run()
{
    SignalHandler signalHandler;

    PluginInfoVector pluginConfigs = read_plugin_configs();

    m_pluginProxies.clear();

    for (auto& info : pluginConfigs)
    {
        m_pluginProxies.emplace_back(std::move(info));
    }

    pluginConfigs.clear();

    for (auto& proxy : m_pluginProxies)
    {
        proxy.ensureStateMatchesOptions();
    }

    bool keepRunning = true;

    setupSocket();

    Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

    Common::ZeroMQWrapper::IHasFDPtr subprocessFD = poller->addEntry(
        signalHandler.subprocessExitFileDescriptor(), Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);
    Common::ZeroMQWrapper::IHasFDPtr terminationFD = poller->addEntry(
        signalHandler.terminationFileDescriptor(), Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);

    poller->addEntry(*m_socket, Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);

    std::chrono::seconds timeout(10);

    while (keepRunning)
    {
        LOGDEBUG("Calling poller at " << ::time(nullptr));
        Common::ZeroMQWrapper::IPoller::poll_result_t active = poller->poll(std::chrono::milliseconds(timeout));
        LOGDEBUG("Returned from poller: " << active.size() << " at " << ::time(nullptr));

        for (auto& fd : active)
        {
            if (fd == terminationFD.get())
            {
                LOGWARN("Sophos watchdog exiting");
                signalHandler.clearTerminationPipe();
                keepRunning = false;
                continue;
            }
            if (fd == subprocessFD.get())
            {
                // Don't log since we don't know if the exit was expected
                signalHandler.clearSubProcessExitPipe();
            }
            if (fd == m_socket.get())
            {
                handleSocketRequest();
            }
        }

        // Child may have exited, or socket request may have altered state.
        timeout = std::chrono::seconds(10);
        for (auto& proxy : m_pluginProxies)
        {
            auto waitPeriod = proxy.checkForExit();
            waitPeriod = std::min(proxy.ensureStateMatchesOptions(), waitPeriod);
            timeout = std::min(waitPeriod, timeout);
        }

        timeout = std::max(timeout, std::chrono::seconds(1)); // Ensure we wait at least 1 second
        LOGDEBUG("timeout = " << timeout.count());
    }

    LOGINFO("Stopping processes");
    for (auto& proxy : m_pluginProxies)
    {
        proxy.stop();
    }

    // Normal shutdown
    m_pluginProxies.clear();
    m_socket.reset();
    m_context.reset();

    return 0;
}

PluginInfoVector Watchdog::read_plugin_configs()
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
        Common::FileSystem::filePermissions()->chmod(ipcFilesPath, S_IRWXU);
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
        return "OK";
    }
    return "Error: Plugin not found";
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
        return "Error: Plugin not found";
    }
    if (proxy == nullptr)
    {
        // Not previously loaded, but now available
        assert(loadResult.second);
        m_pluginProxies.emplace_back(std::move(loadResult.first));
        proxy = &m_pluginProxies.back();
    }
    else if (loadResult.second)
    {
        // update info from disk
        proxy->updatePluginInfo(loadResult.first);
    }
    proxy->setEnabled(true);
    return "OK";
}

std::string Watchdog::removePlugin(const std::string& pluginName)
{
    LOGINFO("Removing " << pluginName);

    bool found = false;
    for (auto it = m_pluginProxies.begin(); it != m_pluginProxies.end();)
    {
        if (pluginName == it->name())
        {
            it->stop();
            it = m_pluginProxies.erase(it);
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
        return "Error: Plugin not found";
    }

    return "OK";
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

    return "Error: Unknown command";
}

PluginProxy* Watchdog::findPlugin(const std::string& pluginName)
{
    for (auto& proxy : m_pluginProxies)
    {
        if (proxy.name() == pluginName)
        {
            return &proxy;
        }
    }
    return nullptr;
}
