/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ManagementAgentMain.h"
#include "Logger.h"

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
#include <ManagementAgent/PluginCommunication/IPluginCommunicationException.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>


using namespace Common;

namespace
{
    static std::unique_ptr<Common::Threads::NotifyPipe> GL_signalPipe;

    static void s_signal_handler (int signal_value)
    {
        LOGDEBUG( "Signal received: " << signal_value );
        if ( !GL_signalPipe)
        {
            return;
        }
        GL_signalPipe->notify();
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

            std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager = std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager>(
                    new ManagementAgent::PluginCommunicationImpl::PluginManager());

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
            sendCurrentPluginsStatus();
        }

        void ManagementAgentMain::loadPlugins()
        {
            // Load known plugins.  New plugins will be loaded via PluginServerCallback
            std::vector<Common::PluginRegistryImpl::PluginInfo> plugins = Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

            for(auto & plugin : plugins)
            {
                m_pluginManager->registerPlugin(plugin.getPluginName());
                LOGINFO("Registered plugin " << plugin.getPluginName() << ", executable path " << plugin.getExecutableFullPath());
                m_pluginManager->setAppIds(plugin.getPluginName(), plugin.getPolicyAppIds(), plugin.getStatusAppIds());
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
            m_policyReceiver = std::make_shared<PolicyReceiverImpl::PolicyReceiverImpl>(m_taskQueue, *m_pluginManager);

            m_statusReceiver = std::make_shared<StatusReceiverImpl::StatusReceiverImpl>(m_taskQueue, m_statusCache);

            m_eventReceiver = std::make_shared<EventReceiverImpl::EventReceiverImpl>(m_taskQueue);

            m_pluginManager->setPolicyReceiver(m_policyReceiver);
            m_pluginManager->setStatusReceiver(m_statusReceiver);
            m_pluginManager->setEventReceiver(m_eventReceiver);
        }

        void ManagementAgentMain::sendCurrentPluginsStatus()
        {
            std::vector<std::string> registeredPlugins = m_pluginManager->getRegisteredPluginNames();

            std::string mcsDir = ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath();
            std::string tempDir = ApplicationConfiguration::applicationPathManager().getTempPath();
            std::string statusDir = ApplicationConfiguration::applicationPathManager().getMcsStatusFilePath();

            for (auto& pluginName : registeredPlugins)
            {
                std::vector<Common::PluginApi::StatusInfo> pluginStatus;
                try
                {
                    pluginStatus = m_pluginManager->getStatus(pluginName);
                }
                catch (PluginCommunication::IPluginCommunicationException& e)
                {
                    LOGWARN("Failed to get plugin status for: " << pluginName << ", with error" << e.what());
                    continue;
                }

                for (auto& pluginStatusInfo : pluginStatus)
                {

                    std::unique_ptr<Common::TaskQueue::ITask>
                            task(
                            new StatusReceiverImpl::StatusTask(
                                    m_statusCache,
                                    pluginStatusInfo.appId,
                                    pluginStatusInfo.statusXml,
                                    pluginStatusInfo.statusWithoutTimestampsXml,
                                    tempDir,
                                    statusDir
                            )
                    );

                    m_taskQueue->queueTask(task);
                }
            }
        }

        int ManagementAgentMain::run()
        {
            LOGDEBUG("Management Agent starting.. ");

            // Setup SIGNAL handling for shutdown.
            Common::ZeroMQWrapper::IHasFDPtr shutdownPipePtr;
            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

            GL_signalPipe = std::unique_ptr<Common::Threads::NotifyPipe>( new Common::Threads::NotifyPipe());
            struct sigaction action;
            action.sa_handler = s_signal_handler;
            action.sa_flags = 0;
            sigemptyset (&action.sa_mask);
            sigaction (SIGINT, &action, NULL);
            sigaction (SIGTERM, &action, NULL);

            shutdownPipePtr = poller->addEntry(GL_signalPipe.get()->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            // start running background threads
            m_taskQueueProcessor->start();
            m_directoryWatcher->startWatch();

            LOGDEBUG("Management Agent running.");

            bool running = true;
            while(running)
            {
                if(GL_signalPipe && GL_signalPipe->notified())
                {
                    LOGDEBUG("Management Agent stopping");
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
