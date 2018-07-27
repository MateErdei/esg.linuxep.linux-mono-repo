/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>

#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/TaskQueue/ITaskProcessor.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>
#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>

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
            int run();


            ManagementAgent::PluginCommunication::IPluginManager* m_pluginManager;

            std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_policyListener;
            std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_actionListener;
            std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;

            std::shared_ptr<ManagementAgent::PluginCommunication::IPolicyReceiver> m_policyReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IStatusReceiver> m_statusReceiver;
            std::shared_ptr<ManagementAgent::PluginCommunication::IEventReceiver> m_eventReceiver;

            std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQueue;
            std::unique_ptr<Common::TaskQueue::ITaskProcessor> m_taskQueueProcessor;

        };
    }
}
