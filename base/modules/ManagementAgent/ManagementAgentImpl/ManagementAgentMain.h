/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITaskProcessor.h>
#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/ZeroMQWrapper/IProxy.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/HealthStatusImpl/HealthStatus.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>
#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>

namespace ManagementAgent
{
    namespace ManagementAgentImpl
    {
        class ManagementAgentMain
        {
        public:
            static int main(int argc, char* argv[]);
            static int mainForValidArguments(bool withPersistentTelemetry=true);
        protected:
            void initialise(ManagementAgent::PluginCommunication::IPluginManager& pluginManager);
            void loadPlugins();
            void initialiseTaskQueue();
            void initialiseDirectoryWatcher();
            void initialisePluginReceivers();
            void sendCurrentPluginPolicies();
            void sendCurrentActions();
            void sendCurrentPluginsStatus(const std::vector<std::string>& registeredPlugins);
            bool updateOngoingWithGracePeriod(unsigned int gracePeriodSeconds);
            bool updateOngoing();
            int run(bool withPersistentTelemetry);

            // to be used in tests
            void test_request_stop();

            ManagementAgent::PluginCommunication::IPluginManager* m_pluginManager;

            std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_policyListener;
            std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_actionListener;
            std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;

            std::shared_ptr<ManagementAgent::PluginCommunication::IPolicyReceiver> m_policyReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IStatusReceiver> m_statusReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IEventReceiver> m_eventReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IThreatHealthReceiver> m_threatHealthReceiver;

            std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQueue;
            std::unique_ptr<Common::TaskQueue::ITaskProcessor> m_taskQueueProcessor;
            std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> m_statusCache;

            /**
             * Remember the original parent PID so that we can exit if it changes.
             */
            pid_t m_ppid;
        };
    } // namespace ManagementAgentImpl
} // namespace ManagementAgent
