/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginManager.h"

#include "PluginServerCallback.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginCommunication/IPluginCommunicationException.h>
#include <Common/PluginCommunicationImpl/PluginProxy.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/PluginCommunication/PluginHealthStatus.h>
#include <sys/stat.h>
#include <json.hpp>

#include <thread>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        PluginManager::PluginManager() :
            m_context(Common::ZMQWrapperApi::createContext()),
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
                    managementSocketAddFilePath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); // NOLINT
            }
            catch (std::exception& exception)
            {
                LOGERROR(
                    "Failed to chmod Management Agent IPC to 660, processes not running as sophos-spl-user will not be "
                    "able to communicate with Management Agent, error:"
                    << exception.what());
            }

            std::shared_ptr<PluginServerCallback> serverCallback = std::make_shared<PluginServerCallback>(*this);
            m_serverCallbackHandler.reset(new PluginServerCallbackHandler(std::move(replier), serverCallback, m_healthStatus));
            m_serverCallbackHandler->start();
        }

        PluginManager::~PluginManager()
        {
            if (m_serverCallbackHandler)
            {
                m_serverCallbackHandler->stop();
            }
        }

        void PluginManager::setServerCallback(
            std::shared_ptr<PluginCommunication::IPluginServerCallback> serverCallback,
            Common::ZeroMQWrapper::ISocketReplierPtr replier)
        {
            if (m_serverCallbackHandler)
            {
                m_serverCallbackHandler->stop();
            }
            m_serverCallbackHandler.reset(
                new PluginServerCallbackHandler(std::move(replier), std::move(serverCallback), m_healthStatus));
            m_serverCallbackHandler->start();
        }

        void PluginManager::setDefaultTimeout(int timeoutMs) { m_defaultTimeout = timeoutMs; }

        void PluginManager::setDefaultConnectTimeout(int timeoutMs) { m_defaultConnectTimeout = timeoutMs; }

        int PluginManager::applyNewPolicy(const std::string& appId, const std::string& policyXml)
        {
            LOGDEBUG("PluginManager: apply new policy: " << appId);
            std::lock_guard<std::mutex> lock(m_pluginMapMutex);

            // Keep a list of plugins we failed to communicate with
            std::vector<std::pair<std::string, std::string>> pluginNamesAndErrors;

            int pluginsNotified = 0;
            for (auto& proxy : m_RegisteredPlugins)
            {
                if (proxy.second->hasPolicyAppId(appId))
                {
                    try
                    {
                        proxy.second->applyNewPolicy(appId, policyXml);
                        ++pluginsNotified;
                    }
                    catch (std::exception& ex)
                    {
                        pluginNamesAndErrors.emplace_back(std::pair<std::string, std::string>(proxy.first, ex.what()));
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
                        pluginNamesAndErrors.emplace_back(std::pair<std::string, std::string>(proxy.first, ex.what()));
                    }
                }
            }

            // extra check for does MA also subscribe to this APPID then ingest/use
            if (appId == "SHS")
            {
                if (isThreatResetTask(actionXml))
                {
                    LOGDEBUG("Processing Health Reset Action.");
                    m_healthStatus->resetThreatDetectionHealth();
                }
            }

            // If we failed to communicate with plugins, check they are still installed. If not, remove them from
            // memory.
            checkPluginRegistry(pluginNamesAndErrors);
            return pluginsNotified;
        }

        void PluginManager::checkPluginRegistry(
            const std::vector<std::pair<std::string, std::string>>& pluginsAndErrors)
        {
            for (auto& plugin : pluginsAndErrors)
            {
                std::string pluginFile = plugin.first + ".json";
                std::string pluginRegistryFileName = Common::FileSystem::join(
                    Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath(), pluginFile);
                if (Common::FileSystem::fileSystem()->isFile(pluginRegistryFileName))
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

        bool PluginManager::isThreatResetTask(std::string filePath)
        {
            auto fs = Common::FileSystem::fileSystem();
            std::string expectedActionFileContents = "type=\"sophos.core.threat.reset\"";
            Path fullPath = Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getMcsActionFilePath(), filePath);
            try
            {
                std::string actionFileContents = fs->readFile(fullPath);
                if (Common::UtilityImpl::StringUtils::isSubstring(actionFileContents, expectedActionFileContents))
                {
                    return true;
                }
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Unable to read Health Action Task at: " << filePath << " due to: " << ex.what());
            }
            return false;
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
            locked_setAppIds(plugin, pluginDetails.policyAppIds, pluginDetails.actionAppIds, pluginDetails.statusAppIds, lock);
            locked_setHealth(plugin, pluginDetails.hasServiceHealth, pluginDetails.hasThreatServiceHealth, pluginDetails.displayName, lock);
        }

        Common::PluginCommunication::IPluginProxy* PluginManager::locked_createPlugin(
            const std::string& pluginName,
            std::lock_guard<std::mutex>&)
        {
            auto requester = m_context->getRequester();
            Common::PluginApiImpl::PluginResourceManagement::setupRequester(
                *requester, pluginName, m_defaultTimeout, m_defaultConnectTimeout);
            std::unique_ptr<Common::PluginCommunication::IPluginProxy> proxyPlugin =
                std::unique_ptr<Common::PluginCommunicationImpl::PluginProxy>(
                    new Common::PluginCommunicationImpl::PluginProxy(std::move(requester), pluginName));
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

        Common::ZMQWrapperApi::IContextSharedPtr PluginManager::getSocketContext() { return m_context; }

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

        void PluginManager::setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver)
        {
            if (m_serverCallbackHandler != nullptr)
            {
                m_serverCallbackHandler->setEventReceiver(receiver);
            }
        }

        void PluginManager::setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver)
        {
            if (m_serverCallbackHandler != nullptr)
            {
                m_serverCallbackHandler->setPolicyReceiver(receiver);
            }
        }

        void PluginManager::setThreatHealthReceiver(
            std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver)
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

        ManagementAgent::PluginCommunication::PluginHealthStatus PluginManager::getHealthStatusForPlugin(const std::string& pluginName, bool prevHealthMissing)
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
                if(healthResult.contains("activeHeartbeatUtmId") && healthResult.contains("activeHeartbeat"))
                {
                    pluginHealthStatus.activeHeartbeatUtmId = healthResult["activeHeartbeatUtmId"];
                    pluginHealthStatus.activeHeartbeat = healthResult["activeHeartbeat"];
                }
            }
            catch(const std::exception& ex)
            {
                LOGWARN("Failed to read plugin health for: " << pluginName << ", with error" << ex.what());
                // default to not running if value is not valid.
                pluginHealthStatus.healthValue = 1;
            }
            return pluginHealthStatus;
        }
        std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> PluginManager::
            getSharedHealthStatusObj()
        {
            return m_healthStatus;
        }

    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
