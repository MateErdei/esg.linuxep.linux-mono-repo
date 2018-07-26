/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ManagementAgentMain.h"
#include "Logger.h"

#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <Common/ZeroMQWrapper/IHasFD.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <signal.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>

using namespace Common;

namespace
{
    static std::unique_ptr<Common::Threads::NotifyPipe> signalPipe;

    static void s_signal_handler (int signal_value)
    {
        LOGDEBUG( "Signal received: " << signal_value );
        if ( !signalPipe)
        {
            return;
        }
        signalPipe->notify();
    }

}


namespace ManagementAgent
{
    namespace ManagementAgentImpl
    {
        int ManagementAgentMain::main(int argc, char **argv)
        {
            static_cast<void>(argv); // unused

            if(argc > 1)
            {
                LOGERROR("Error, invalid command line arguments. Usage: Management Agent");
                return -1;
            }

            ManagementAgent::PluginCommunication::IPluginManager* pluginManager = new ManagementAgent::PluginCommunicationImpl::PluginManager();

            ManagementAgentMain managementAgent;
            managementAgent.initialise(*pluginManager);
            return managementAgent.run();
        }

        void ManagementAgentMain::initialise(ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
        {
            LOGDEBUG("Initializing Management Agent");

            m_pluginManager = &pluginManager;

            // order is important.
            loadPlugins();
            initialiseTaskQueue();
            initialiseDirectoryWatcher();
            initialisePluginReceivers();
        }

        void ManagementAgentMain::loadPlugins()
        {
            std::vector<Common::PluginRegistryImpl::PluginInfo> plugins = Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

            for(auto & plugin : plugins)
            {
                m_pluginManager->registerPlugin(plugin.getPluginName());
                LOGINFO("Registered plugin " << plugin.getPluginName() << ", executable path " << plugin.getExecutableFullPath());
            }
        }

        void ManagementAgentMain::initialiseTaskQueue()
        {
            m_taskQueue = std::make_shared<TaskQueueImpl::TaskQueueImpl>();
            m_taskQueueProcessor = std::unique_ptr<TaskQueue::ITaskProcessor>(
                    new TaskQueueImpl::TaskProcessorImpl(m_taskQueue));
        }

        void ManagementAgentMain::initialiseDirectoryWatcher()
        {
            m_policyListener = std::unique_ptr<McsRouterPluginCommunicationImpl::TaskDirectoryListener>
                    (new McsRouterPluginCommunicationImpl::TaskDirectoryListener(ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath(), m_taskQueue, *m_pluginManager));
            m_actionListener = std::unique_ptr<McsRouterPluginCommunicationImpl::TaskDirectoryListener>
                    (new McsRouterPluginCommunicationImpl::TaskDirectoryListener(ApplicationConfiguration::applicationPathManager().getMcsActionFilePath(), m_taskQueue, *m_pluginManager));

            m_directoryWatcher = std::unique_ptr<DirectoryWatcher::IDirectoryWatcher>(new DirectoryWatcherImpl::DirectoryWatcher());
            m_directoryWatcher->addListener(*m_policyListener);
            m_directoryWatcher->addListener(*m_actionListener);

        }

        void ManagementAgentMain::initialisePluginReceivers()
        {
            m_policyReceiver = std::make_shared<PolicyReceiverImpl::PolicyReceiverImpl>(
                    ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath(), m_taskQueue, *m_pluginManager);

            m_statusReceiver = std::make_shared<StatusReceiverImpl::StatusReceiverImpl>(
                    ApplicationConfiguration::applicationPathManager().getMcsStatusFilePath(), m_taskQueue);

            m_eventReceiver = std::make_shared<EventReceiverImpl::EventReceiverImpl>(
                    ApplicationConfiguration::applicationPathManager().getMcsEventFilePath(), m_taskQueue);

            m_pluginManager->setPolicyReceiver(m_policyReceiver);
            m_pluginManager->setStatusReceiver(m_statusReceiver);
            m_pluginManager->setEventReceiver(m_eventReceiver);
        }

        int ManagementAgentMain::run()
        {
            LOGDEBUG("Management Agent starting.. ");

            // Setup SIGNAL handling for shutdown.
            Common::ZeroMQWrapper::IHasFDPtr shutdownPipePtr;
            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

            signalPipe = std::unique_ptr<Common::Threads::NotifyPipe>( new Common::Threads::NotifyPipe());
            struct sigaction action;
            action.sa_handler = s_signal_handler;
            action.sa_flags = 0;
            sigemptyset (&action.sa_mask);
            sigaction (SIGINT, &action, NULL);
            sigaction (SIGTERM, &action, NULL);

            shutdownPipePtr = poller->addEntry(signalPipe.get()->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            // start running background threads
            m_taskQueueProcessor->start();
            m_directoryWatcher->startWatch();

            LOGDEBUG("Management Agent running.");

            bool running = true;
            while(running)
            {
                if(signalPipe && signalPipe->notified())
                {
                    running = false;
                }
            }

            // pre-pare and stop back ground threads
            m_directoryWatcher->removeListener(*m_policyListener);
            m_directoryWatcher->removeListener(*m_actionListener);
            m_taskQueueProcessor->stop();

            LOGDEBUG("Management Agent stopped");
            return 0;
        }


    }
}
