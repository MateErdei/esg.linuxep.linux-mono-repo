/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>
#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>
#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/TaskQueue/ITaskProcessor.h>

namespace ManagementAgent
{
    namespace ManagementAgentImpl
    {

        class ManagementAgentMain
        {
        public:
            static int main(int argc, char *argv[]);

        protected:
            void initialise(ManagementAgent::PluginCommunication::IPluginManager& pluginManager);
            void loadPlugins();
            void initialiseTaskQueue();
            void initialiseDirectoryWatcher();
            void initialisePluginReceivers();
            void sendCurrentPluginPolicies(const std::vector<std::string>& registeredPlugins);
            void sendCurrentActions(const std::vector<std::string>& registeredPlugins);
            void sendCurrentPluginsStatus(const std::vector<std::string>& registeredPlugins);
            int run();
            // to be used in tests
            void test_request_stop();

            ManagementAgent::PluginCommunication::IPluginManager* m_pluginManager;

            std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_policyListener;
            std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_actionListener;
            std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;

            std::shared_ptr<ManagementAgent::PluginCommunication::IPolicyReceiver> m_policyReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IStatusReceiver> m_statusReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IEventReceiver> m_eventReceiver;

            std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQueue;
            std::unique_ptr<Common::TaskQueue::ITaskProcessor> m_taskQueueProcessor;
            std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> m_statusCache;


            /**
             * Remember the original parent PID so that we can exit if it changes.
             */
            pid_t m_ppid;

        };
        
        // Design decision to create a binary incompatible version of ManagementAgent
        void reportMy999Version();
    }
}
