// Copyright 2018-2024 Sophos Limited. All rights reserved.

#include "PluginManager.h"

#include "PluginServerCallback.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/PluginCommunication/IPluginCommunicationException.h"
#include "Common/PluginCommunicationImpl/PluginProxy.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "ManagementAgent/LoggerImpl/Logger.h"

#include <sys/stat.h>

#include <nlohmann/json.hpp>
#include <memory>
#include <thread>

namespace
{
    std::string readAction(const std::string& filename)
    {
        auto fs = Common::FileSystem::fileSystem();
        Path fullPath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getMcsActionFilePath(), filename);
        try
        {
            return fs->readFile(fullPath);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Unable to read Health Action Task at: " << fullPath << " due to: " << ex.what());
        }
        return "";
    }
} // namespace

namespace ManagementAgent::PluginCommunicationImpl
{
    PluginManager::PluginManager(Common::ZMQWrapperApi::IContextSharedPtr context) :
        m_context(std::move(context)),
        m_healthStatus(std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>()),
        m_serverCallbackHandler(),
        m_defaultTimeout(5000),
        m_defaultConnectTimeout(5000)
    {
        auto replier = m_context->getReplier();
        setTimeouts(*replier);
        std::string managementSocketAdd =
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(managementSocketAdd);

        try
        {
            // Strip off "ipc://" to get actual file path from IPC path.
            std::string managementSocketAddFilePath = managementSocketAdd.substr(6);
            Common::FileSystem::filePermissions()->chmod(
                managementSocketAddFilePath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        }
        catch (std::exception& exception)
        {
            LOGERROR(
                "Failed to chmod Management Agent IPC to 660, processes not running as sophos-spl-user will not be "
                "able to communicate with Management Agent, error:"
                << exception.what());
        }

        auto serverCallback = std::make_shared<PluginServerCallback>(*this);
        setServerCallback(std::move(serverCallback), std::move(replier));
    }

    PluginManager::~PluginManager()
    {
        if (m_serverCallbackHandler)
        {
            // Need to stop and join the reactor before we delete it
            m_serverCallbackHandler->stopAndJoin();
        }
    }

    void PluginManager::setServerCallback(
        ServerCallbackPtr serverCallback,
        Common::ZeroMQWrapper::ISocketReplierPtr replier)
    {
        if (m_serverCallbackHandler)
        {
            // About to delete current m_serverCallbackHandler so need to stop and join reactor
            m_serverCallbackHandler->stopAndJoin();
        }
        m_serverCallbackHandler = std::make_unique<PluginServerCallbackHandler>(
            std::move(replier), std::move(serverCallback), m_healthStatus);
        m_serverCallbackHandler->start();
    }

    void PluginManager::setDefaultTimeout(int timeoutMs)
    {
        m_defaultTimeout = timeoutMs;
    }

    void PluginManager::setDefaultConnectTimeout(int timeoutMs)
    {
        m_defaultConnectTimeout = timeoutMs;
    }

    int PluginManager::applyNewPolicy(
        const std::string& appId,
        const std::string& policyXml,
        const std::string& pluginName)
    {
        if (pluginName.empty())
        {
            LOGDEBUG("PluginManager: apply new policy: " << appId);
        }
        else
        {
            LOGDEBUG("PluginManager: apply new policy: " << appId << " to plugin: " << pluginName);
        }

        std::lock_guard<std::mutex> lock(m_pluginMapMutex);

        // Keep a list of plugins we failed to communicate with
        std::vector<std::pair<std::string, std::string>> pluginNamesAndErrors;

        int pluginsNotified = 0;
        for (auto& [registeredPluginName, registeredProxy] : m_RegisteredPlugins)
        {
            if (registeredProxy->hasPolicyAppId(appId) && (pluginName.empty() || registeredPluginName == pluginName))
            {
                try
                {
                    registeredProxy->applyNewPolicy(appId, policyXml);
                    ++pluginsNotified;
                }
                catch (std::exception& ex)
                {
                    pluginNamesAndErrors.emplace_back(registeredPluginName, ex.what());
                }
            }
        }
        // If we failed to communicate with plugins, check they are still installed. If not, remove them from
        // memory.
        checkPluginRegistry(pluginNamesAndErrors);
        return pluginsNotified;
    }

    int PluginManager::queueAction(
        const std::string& appId,
        const std::string& actionXml,
        const std::string& correlationId)
    {
        LOGDEBUG("PluginManager: Queue action " << appId);
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);

        // Keep a list of plugins we failed to communicate with
        std::vector<std::pair<std::string, std::string>> pluginNamesAndErrors;

        int pluginsNotified = 0;
        for (auto& proxy : m_RegisteredPlugins)
        {
            if (proxy.second->hasActionAppId(appId))
            {
                try
                {
                    proxy.second->queueAction(appId, actionXml, correlationId);
                    ++pluginsNotified;
                }
                catch (std::exception& ex)
                {
                    pluginNamesAndErrors.emplace_back(proxy.first, ex.what());
                }
            }
        }

        // extra check for does MA also subscribe to this APPID then ingest/use
        if (appId == "CORE")
        {
            std::string xml = readAction(actionXml); // Actually filename
            // Reset outbreak mode first, in case health status regenerates health
            if (eventReceiver_)
            {
                eventReceiver_->handleAction(xml);
                assert(m_healthStatus);
                m_healthStatus->setOutbreakMode(eventReceiver_->outbreakMode());
            }
            if (isThreatResetTask(xml))
            {
                LOGDEBUG("Processing Health Reset Action.");
                assert(m_healthStatus);
                m_healthStatus->resetThreatDetectionHealth();
            }
        }

        // If we failed to communicate with plugins, check they are still installed. If not, remove them from
        // memory.
        checkPluginRegistry(pluginNamesAndErrors);
        return pluginsNotified;
    }

    void PluginManager::checkPluginRegistry(const std::vector<std::pair<std::string, std::string>>& pluginsAndErrors)
    {
        for (auto& plugin : pluginsAndErrors)
        {
            if (checkIfSinglePluginInRegistry(plugin.first))
            {
                // Plugin is installed, but we failed to communicate with it. Log an error.
                LOGERROR("Failure on sending message to " << plugin.first << ". Reason: " << plugin.second);
            }
            else
            {
                // Plugin is no longer installed, remove it from memory
                m_RegisteredPlugins.erase(plugin.first);
                LOGINFO(plugin.first << " has been uninstalled.");
            }
        }
    }

    bool PluginManager::checkIfSinglePluginInRegistry(const std::string& pluginName)
    {
        std::string pluginFile = pluginName + ".json";
        std::string pluginRegistryFileName = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath(), pluginFile);
        return (Common::FileSystem::fileSystem()->isFile(pluginRegistryFileName));
    }

    std::vector<Common::PluginApi::StatusInfo> PluginManager::getStatus(const std::string& pluginName)
    {
        LOGDEBUG("PluginManager: get status " << pluginName);
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        return getPlugin(pluginName)->getStatus();
    }

    std::string PluginManager::getTelemetry(const std::string& pluginName)
    {
        LOGDEBUG("PluginManager: get telemetry " << pluginName);
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        return getPlugin(pluginName)->getTelemetry();
    }

    std::string PluginManager::getHealth(const std::string& pluginName)
    {
        LOGDEBUG("PluginManager: get health " << pluginName);
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        return getPlugin(pluginName)->getHealth();
    }

    bool PluginManager::isThreatResetTask(const std::string& xml)
    {
        if (xml.empty())
        {
            return false;
        }
        std::string expectedActionFileContents = "type=\"sophos.core.threat.reset\"";
        return Common::UtilityImpl::StringUtils::isSubstring(xml, expectedActionFileContents);
    }

    void PluginManager::locked_setAppIds(
        Common::PluginCommunication::IPluginProxy* plugin,
        const std::vector<std::string>& policyAppIds,
        const std::vector<std::string>& actionAppIds,
        const std::vector<std::string>& statusAppIds,
        std::lock_guard<std::mutex>&)
    {
        std::string firstPolicy = policyAppIds.empty() ? "None" : policyAppIds.at(0).c_str();
        LOGDEBUG("PluginManager: associate appids to pluginName " << plugin->name() << ": " << firstPolicy);

        plugin->setPolicyAppIds(policyAppIds);
        plugin->setActionAppIds(actionAppIds);
        plugin->setStatusAppIds(statusAppIds);
    }

    void PluginManager::locked_setHealth(
        Common::PluginCommunication::IPluginProxy* plugin,
        bool serviceHealth,
        bool threatHealth,
        const std::string& displayName,
        std::lock_guard<std::mutex>&)
    {
        plugin->setServiceHealth(serviceHealth);
        plugin->setThreatServiceHealth(threatHealth);
        plugin->setDisplayPluginName(displayName);
    }

    void PluginManager::registerPlugin(const std::string& pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        locked_createPlugin(pluginName, lock);
    }

    void PluginManager::registerAndConfigure(
        const std::string& pluginName,
        const PluginCommunication::PluginDetails& pluginDetails)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        auto plugin = locked_createPlugin(pluginName, lock);
        locked_setAppIds(
            plugin, pluginDetails.policyAppIds, pluginDetails.actionAppIds, pluginDetails.statusAppIds, lock);
        locked_setHealth(
            plugin,
            pluginDetails.hasServiceHealth,
            pluginDetails.hasThreatServiceHealth,
            pluginDetails.displayName,
            lock);
    }

    Common::PluginCommunication::IPluginProxy* PluginManager::locked_createPlugin(
        const std::string& pluginName,
        std::lock_guard<std::mutex>&)
    {
        auto requester = m_context->getRequester();
        Common::PluginApiImpl::PluginResourceManagement::setupRequester(
            *requester, pluginName, m_defaultTimeout, m_defaultConnectTimeout);
        std::unique_ptr<Common::PluginCommunication::IPluginProxy> proxyPlugin =
            std::make_unique<Common::PluginCommunicationImpl::PluginProxy>(std::move(requester), pluginName);
        m_RegisteredPlugins[pluginName] = std::move(proxyPlugin);
        return m_RegisteredPlugins[pluginName].get();
    }

    Common::PluginCommunication::IPluginProxy* PluginManager::getPlugin(const std::string& pluginName)
    {
        auto found = m_RegisteredPlugins.find(pluginName);
        if (found != m_RegisteredPlugins.end())
        {
            return found->second.get();
        }
        throw Common::PluginCommunication::IPluginCommunicationException("Tried to access non-registered plugin");
    }

    void PluginManager::removePlugin(const std::string& pluginName)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        m_RegisteredPlugins.erase(pluginName);
    }

    Common::ZMQWrapperApi::IContextSharedPtr PluginManager::getSocketContext()
    {
        return m_context;
    }

    void PluginManager::setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket)
    {
        socket.setTimeout(m_defaultTimeout);
        socket.setConnectionTimeout(m_defaultConnectTimeout);
    }

    void PluginManager::setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
    {
        if (m_serverCallbackHandler != nullptr)
        {
            m_serverCallbackHandler->setStatusReceiver(statusReceiver);
        }
    }

    void PluginManager::setEventReceiver(PluginCommunication::IEventReceiverPtr receiver)
    {
        eventReceiver_ = std::move(receiver);
        assert(m_healthStatus);
        m_healthStatus->setOutbreakMode(eventReceiver_->outbreakMode());

        if (m_serverCallbackHandler != nullptr)
        {
            m_serverCallbackHandler->setEventReceiver(eventReceiver_);
        }
    }

    void PluginManager::setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver)
    {
        if (m_serverCallbackHandler != nullptr)
        {
            m_serverCallbackHandler->setPolicyReceiver(receiver);
        }
    }

    void PluginManager::setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver)
    {
        if (m_serverCallbackHandler != nullptr)
        {
            m_serverCallbackHandler->setThreatHealthReceiver(receiver);
        }
    }

    std::vector<std::string> PluginManager::getRegisteredPluginNames()
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        std::vector<std::string> pluginNameList;

        for (auto& plugin : m_RegisteredPlugins)
        {
            pluginNameList.push_back(plugin.first);
        }

        return pluginNameList;
    }

    ManagementAgent::PluginCommunication::PluginHealthStatus PluginManager::getHealthStatusForPlugin(
        const std::string& pluginName,
        bool prevHealthMissing)
    {
        std::lock_guard<std::mutex> lock(m_pluginMapMutex);
        auto plugin = getPlugin(pluginName);

        bool serviceHealth = plugin->getServiceHealth();
        bool threatServiceHealth = plugin->getThreatServiceHealth();
        std::string displayName = plugin->getDisplayPluginName();

        ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthStatus;
        if (serviceHealth && threatServiceHealth)
        {
            pluginHealthStatus.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE_AND_THREAT;
        }
        else if (serviceHealth)
        {
            pluginHealthStatus.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
        }
        else if (threatServiceHealth)
        {
            pluginHealthStatus.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
        }
        else
        {
            return pluginHealthStatus;
        }
        pluginHealthStatus.displayName = displayName;
        std::string health;

        try
        {
            health = plugin->getHealth();
            if (prevHealthMissing)
            {
                LOGINFO("Health restored for service " << plugin->getDisplayPluginName());
            }
        }
        catch (const Common::PluginCommunication::IPluginCommunicationException&)
        {
            if (!prevHealthMissing)
            {
                LOGWARN("Could not get health for service " << plugin->getDisplayPluginName());
            }

            pluginHealthStatus.healthValue = 2;
            return pluginHealthStatus;
        }

        try
        {
            nlohmann::json healthResult = nlohmann::json::parse(health);
            pluginHealthStatus.healthValue = healthResult["Health"];
            if (healthResult.contains("activeHeartbeatUtmId") && healthResult.contains("activeHeartbeat"))
            {
                pluginHealthStatus.activeHeartbeatUtmId = healthResult["activeHeartbeatUtmId"];
                pluginHealthStatus.activeHeartbeat = healthResult["activeHeartbeat"];
            }
            if (healthResult.contains("Isolation"))
            {
                pluginHealthStatus.isolated = healthResult["Isolation"];
            }
        }
        catch (const std::exception& ex)
        {
            LOGWARN("Failed to read plugin health for: " << pluginName << ", with error: " << ex.what());
            // default to not running if value is not valid.
            pluginHealthStatus.healthValue = 1;
        }
        return pluginHealthStatus;
    }

    std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> PluginManager::getSharedHealthStatusObj()
    {
        return m_healthStatus;
    }

    bool PluginManager::updateOngoing()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string markerFile = Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
        if (fs->isFile(markerFile))
        {
            return true;
        }
        return false;
    }

    bool PluginManager::updateOngoingWithGracePeriod(unsigned int gracePeriodSeconds, timepoint_t now)
    {
        static timepoint_t lastTimeWeSawUpdateMarker{};
        if (updateOngoing())
        {
            lastTimeWeSawUpdateMarker = now;
            return true;
        }

        // If it's been gracePeriodSeconds seconds since we last saw the update marker and it's not there anymore,
        // then we make sure to give the specified grace period for updates to finish.
        return (now - lastTimeWeSawUpdateMarker) < std::chrono::seconds(gracePeriodSeconds);
    }
} // namespace ManagementAgent::PluginCommunicationImpl
