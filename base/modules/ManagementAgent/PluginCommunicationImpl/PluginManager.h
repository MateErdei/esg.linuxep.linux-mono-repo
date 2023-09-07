// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginServerCallbackHandler.h"

#include "Common/PluginCommunication/IPluginProxy.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"
#include "Common/ZeroMQWrapper/IProxy.h"
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"
#include "ManagementAgent/HealthStatusImpl/HealthStatus.h"
#include "ManagementAgent/PluginCommunication/IPluginManager.h"
#include "ManagementAgent/PluginCommunication/IPluginServerCallback.h"

#include <map>
#include <mutex>
#include <string>

namespace ManagementAgent::PluginCommunicationImpl
{
    class PluginManager : virtual public PluginCommunication::IPluginManager
    {
    public:
        using ServerCallback_t = PluginCommunicationImpl::PluginServerCallbackHandler::ServerCallback_t;
        using ServerCallbackPtr = std::shared_ptr<ServerCallback_t>;

        explicit PluginManager(Common::ZMQWrapperApi::IContextSharedPtr context);
        ~PluginManager() override;

        int applyNewPolicy(const std::string& appId, const std::string& policyXml, const std::string& pluginName)
            override;
        int queueAction(const std::string& appId, const std::string& actionXml, const std::string& correlationId)
            override;
        void checkPluginRegistry(const std::vector<std::pair<std::string, std::string>>& pluginsAndErrors);
        bool checkIfSinglePluginInRegistry(const std::string& pluginName) override;
        std::vector<Common::PluginApi::StatusInfo> getStatus(const std::string& pluginName) override;
        std::string getTelemetry(const std::string& pluginName) override;
        std::string getHealth(const std::string& pluginName) override;

        void registerPlugin(const std::string& pluginName) override;
        void registerAndConfigure(
            const std::string& pluginName,
            const PluginCommunication::PluginDetails& pluginDetails) override;

        void removePlugin(const std::string& pluginName) override;

        std::vector<std::string> getRegisteredPluginNames() override;

        Common::ZMQWrapperApi::IContextSharedPtr getSocketContext();
        void setServerCallback(ServerCallbackPtr pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replierPtr);

        void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver) override;

        void setEventReceiver(PluginCommunication::IEventReceiverPtr receiver) override;

        void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver) override;
        void setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver) override;

        ManagementAgent::PluginCommunication::PluginHealthStatus getHealthStatusForPlugin(
            const std::string& pluginName,
            bool prevHealthMissing) override;

        std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> getSharedHealthStatusObj() override;

        /**
         * Used mainly for Tests
         */
        void setDefaultTimeout(int timeoutMs);
        void setDefaultConnectTimeout(int timeoutMs);

        /**
         * Are we in a graceperiod during an update (or during the update itself)
         * @param gracePeriodSeconds
         * @param now
         * @return True if within grace period
         */
        bool updateOngoingWithGracePeriod(unsigned int gracePeriodSeconds, timepoint_t now) override;

    private:
        Common::PluginCommunication::IPluginProxy* getPlugin(const std::string& pluginName);

        void setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket);

        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::IProxyPtr m_proxy;
        std::map<std::string, std::unique_ptr<Common::PluginCommunication::IPluginProxy>> m_RegisteredPlugins;
        std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> m_healthStatus;
        std::unique_ptr<PluginServerCallbackHandler> m_serverCallbackHandler;
        PluginCommunication::IEventReceiverPtr eventReceiver_;
        int m_defaultTimeout;
        int m_defaultConnectTimeout;
        std::mutex m_pluginMapMutex;

        /**
         * Create a plugin, always re-creating it
         * Must already hold m_pluginMapMutex
         * @param pluginName
         * @param lock Force a lock_guard to be passed in to verify that we have locked a mutex
         * @return BORROWED pointer to the proxy
         */
        Common::PluginCommunication::IPluginProxy* locked_createPlugin(
            const std::string& pluginName,
            std::lock_guard<std::mutex>& lock);

        /**
         *
         * Must already hold m_pluginMapMutex
         *
         * @param pluginName
         * @param policyAppIds
         * @param statusAppIds
         * @param lock Force a lock_guard to be passed in to verify that we have locked a mutex
         */
        static void locked_setAppIds(
            Common::PluginCommunication::IPluginProxy* plugin,
            const std::vector<std::string>& policyAppIds,
            const std::vector<std::string>& actionAppIds,
            const std::vector<std::string>& statusAppIds,
            std::lock_guard<std::mutex>& lock);

        /**
         * Must already hold m_pluginMapMutex
         *
         * @param plugin
         * @param serviceHealth
         * @param threatHealth
         * @param displayName
         * @param lock
         */
        static void locked_setHealth(
            Common::PluginCommunication::IPluginProxy* plugin,
            bool serviceHealth,
            bool threatHealth,
            const std::string& displayName,
            std::lock_guard<std::mutex>& lock);

        /**
         * Threat resets are passed through the Task Queue but need to be acted on by Management Agent.
         * This function checks an Action file's contents to see if it's a Health Reset.
         *
         * @param filePath
         * @return true if task is threat reset
         */
        static bool isThreatResetTask(const std::string& filePath);

        bool updateOngoing();
    };
} // namespace ManagementAgent::PluginCommunicationImpl
