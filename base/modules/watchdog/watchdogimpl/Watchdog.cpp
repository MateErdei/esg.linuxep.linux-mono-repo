// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Watchdog.h"

#include "Logger.h"
#include "PluginProxy.h"
#include "UserGroupUtils.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "Common/ProcessMonitoringImpl/SignalHandler.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/UtilityImpl/ConfigException.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/WatchdogConstants/WatchdogConstants.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/IPoller.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"

#include <sys/select.h>
#include <sys/stat.h>

#include <cassert>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <memory>
#include <set>
#include <unistd.h>

namespace
{
    const char* PLUGINNOTFOUND = "Error: Plugin not found";
    enum class PluginStatus
    {
        NotFound,
        NotRunning,
        Running,
        Disabled
    };
} // namespace

using namespace watchdog::watchdogimpl;

Watchdog::Watchdog(Common::ZMQWrapperApi::IContextSharedPtr context) :
    Common::ProcessMonitoringImpl::ProcessMonitor(context),
    m_watchdogservice(context, std::bind(&Watchdog::getListOfPluginNames, this))
{
}

Watchdog::Watchdog() : Watchdog(Common::ZMQWrapperApi::createContext()) {}

Watchdog::~Watchdog() = default;

int Watchdog::initialiseAndRun()
{
    try
    {
        PluginInfoVector pluginConfigs = readPluginConfigs();

        for (auto& info : pluginConfigs)
        {
            addProcessToMonitor(std::make_unique<PluginProxy>(std::move(info)));
        }

        pluginConfigs.clear();
        writeExecutableUserAndGroupToActualUserGroupIdConfig();
        reconfigureUserAndGroupIds();

        setupSocket();

        addReplierSocketAndHandleToPoll(m_socket.get(), [this]() { this->handleSocketRequest(); });
    }
    catch (std::exception& ex)
    {
        throw Common::UtilityImpl::ConfigException("Watchdog", ex.what());
    }

    run();

    // Normal shutdown
    m_socket.reset();
    return 0;
}

PluginInfoVector Watchdog::readPluginConfigs()
{
    return Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();
}

std::vector<std::string> Watchdog::getListOfPluginNames()
{
    return ProcessMonitor::getListOfPluginNames();
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
    PluginStatus status{ PluginStatus::NotFound };
    auto functor = [&status](Common::ProcessMonitoring::IProcessProxy& processProxy)
    {
        processProxy.setEnabled(false);
        if (std::getenv("SOPHOS_CORE_DUMP_ON_PLUGIN_KILL"))
        {
            processProxy.setCoreDumpMode(true);
        }

        status = PluginStatus::Disabled;
    };

    ProcessMonitor::applyToProcessProxy(pluginName, functor);

    if (status == PluginStatus::Disabled)
    {
        return Common::WatchdogConstants::Response::watchdogReturnsOk;
    }
    return PLUGINNOTFOUND;
}

std::string Watchdog::enablePlugin(const std::string& pluginName)
{
    LOGINFO("Starting " << pluginName);
    bool pluginIsManaged = false;
    auto detectPluginIsManaged = [&pluginIsManaged](Common::ProcessMonitoring::IProcessProxy&)
    { pluginIsManaged = true; };

    auto enablePlugin = [](Common::ProcessMonitoring::IProcessProxy& processProxy) { processProxy.setEnabled(true); };

    ProcessMonitor::applyToProcessProxy(pluginName, detectPluginIsManaged);

    std::pair<Common::PluginRegistryImpl::PluginInfo, bool> loadResult =
        Common::PluginRegistryImpl::PluginInfo::loadPluginInfoFromRegistry(pluginName);

    if (!pluginIsManaged && !loadResult.second)
    {
        // No plugin loaded and none on disk
        return PLUGINNOTFOUND;
    }
    if (!pluginIsManaged)
    {
        // Not previously loaded, but now available
        assert(loadResult.second);
        addProcessToMonitor(std::make_unique<PluginProxy>(std::move(loadResult.first)));
        writeExecutableUserAndGroupToActualUserGroupIdConfig();
    }
    else if (loadResult.second)
    {
        // update info from disk

        auto infoUpdater = [&loadResult, this](Common::ProcessMonitoring::IProcessProxy& processProxy)
        {
            auto* proxy = dynamic_cast<PluginProxy*>(&processProxy);
            if (proxy != nullptr)
            {
                writeExecutableUserAndGroupToActualUserGroupIdConfig();
                bool changed = proxy->updatePluginInfo(loadResult.first);
                if (changed && proxy->isRunning())
                {
                    LOGINFO("Plugin info changed while plugin running so stopping plugin");
                    proxy->stop();
                }
            }
            processProxy.setEnabled(true);
        };
        applyToProcessProxy(pluginName, infoUpdater);
        return Common::WatchdogConstants::Response::watchdogReturnsOk;
    }

    ProcessMonitor::applyToProcessProxy(pluginName, enablePlugin);
    return Common::WatchdogConstants::Response::watchdogReturnsOk;
}

std::string Watchdog::removePlugin(const std::string& pluginName)
{
    LOGINFO("Removing " << pluginName);

    bool found = ProcessMonitor::removePluginByName(pluginName);
    writeExecutableUserAndGroupToActualUserGroupIdConfig();

    if (!found)
    {
        // Maybe should just return "OK"?
        return PLUGINNOTFOUND;
    }

    return Common::WatchdogConstants::Response::watchdogReturnsOk;
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

std::string Watchdog::checkPluginIsRunning(const std::string& pluginName)
{
    PluginStatus status{ PluginStatus::NotFound };
    auto functor = [&status](Common::ProcessMonitoring::IProcessProxy& processProxy)
    {
        if (processProxy.isRunning())
        {
            status = PluginStatus::Running;
        }
        else
        {
            status = PluginStatus::NotRunning;
        }
    };

    ProcessMonitor::applyToProcessProxy(pluginName, functor);

    switch (status)
    {
        case PluginStatus::NotRunning:
            return Common::WatchdogConstants::Response::watchdogReturnsNotRunning;
        case PluginStatus::Running:
            return Common::WatchdogConstants::Response::watchdogReturnsOk;
        case PluginStatus::NotFound:
        default:
            return PLUGINNOTFOUND;
    }
}

void Watchdog::writeExecutableUserAndGroupToActualUserGroupIdConfig()
{
    auto fileSystem = Common::FileSystem::fileSystem();
    auto filePermissions = Common::FileSystem::filePermissions();
    std::string actualUserGroupIdConfigPath =
        Common::ApplicationConfiguration::applicationPathManager().getActualUserGroupIdConfigPath();

    std::set<std::string> groups{ "sophos-spl-ipc" };
    std::set<std::string> users;

    PluginInfoVector pluginConfigs = readPluginConfigs();
    for (const auto& config : pluginConfigs)
    {
        std::string executableUserAndGroupAsString = config.getExecutableUserAndGroupAsString();

        if (Common::UtilityImpl::StringUtils::isSubstring(executableUserAndGroupAsString, ":"))
        {
            std::vector<std::string> userAndGroup =
                Common::UtilityImpl::StringUtils::splitString(executableUserAndGroupAsString, ":");
            if (userAndGroup.size() == 2 && !userAndGroup[1].empty())
            {
                std::string userName = userAndGroup[0];
                std::string groupName = userAndGroup[1];

                if (userName != "root")
                {
                    users.insert(userName);
                }

                if (groupName != "root")
                {
                    groups.insert(groupName);
                }
            }
        }
        else
        {
            if (executableUserAndGroupAsString != "root")
            {
                users.insert(executableUserAndGroupAsString);
            }
        }
    }

    try
    {
        nlohmann::json userGroupIdConfig;
        if (fileSystem->isFile(actualUserGroupIdConfigPath))
        {
            LOGDEBUG("Overwriting existing user and group ID config " << actualUserGroupIdConfigPath);
        }

        for (const std::string& user : users)
        {
            userGroupIdConfig["users"][user] = filePermissions->getUserId(user);
        }
        for (const std::string& group : groups)
        {
            userGroupIdConfig["groups"][group] = filePermissions->getGroupId(group);
        }

        LOGDEBUG("Updating actual user and group ID config: " << userGroupIdConfig.dump());
        fileSystem->writeFile(actualUserGroupIdConfigPath, userGroupIdConfig.dump(4));
    }
    catch (const Common::FileSystem::IFileSystemException& error)
    {
        LOGERROR(error.what());
    }
}

void Watchdog::reconfigureUserAndGroupIds()
{
    try
    {
        auto changesNeeded = readRequestedUserGroupIds();

        if (!changesNeeded.empty())
        {
            LOGDEBUG("Request to reconfigure the following user and group IDs: " << changesNeeded.dump());
            applyUserIdConfig(changesNeeded);
            writeExecutableUserAndGroupToActualUserGroupIdConfig();
        }
    }
    catch (const std::exception& exception)
    {
        LOGERROR("Failed to reconfigure user and group IDs: " << exception.what());
    }
}
