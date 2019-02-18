/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ManagementAgentMain.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <Common/ZeroMQWrapper/IHasFD.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapper/IProxy.h>
#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/PluginCommunication/IPluginCommunicationException.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <csignal>

using namespace Common;

namespace
{
    std::unique_ptr<Common::Threads::NotifyPipe> GL_signalPipe;

    void s_signal_handler(int)
    {
        if (!GL_signalPipe)
        {
            return;
        }
        GL_signalPipe->notify();
    }

} // namespace

namespace ManagementAgent
{
    namespace ManagementAgentImpl
    {
        int ManagementAgentMain::main(int argc, char** argv)
        {
            umask(S_IRWXG | S_IRWXO); // Read and write for the owner
            static_cast<void>(argv);  // unused
            Common::Logging::FileLoggingSetup loggerSetup("sophos_managementagent", true);
            if (argc > 1)
            {
                LOGERROR("Error, invalid command line arguments. Usage: Management Agent");
                return -1;
            }

            std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager =
                std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager>(
                    new ManagementAgent::PluginCommunicationImpl::PluginManager());

            ManagementAgentMain managementAgent;
            managementAgent.initialise(*pluginManager);
            return managementAgent.run();
        }

        void ManagementAgentMain::initialise(ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
        {
            LOGINFO("Initializing Management Agent");

            m_pluginManager = &pluginManager;
            m_statusCache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
            m_statusCache->loadCacheFromDisk();
            // order is important.
            loadPlugins();
            initialiseTaskQueue();
            initialiseDirectoryWatcher();
            initialisePluginReceivers();
            std::vector<std::string> registeredPlugins = m_pluginManager->getRegisteredPluginNames();
            sendCurrentPluginPolicies();
            sendCurrentPluginsStatus(registeredPlugins);
            sendCurrentActions();

        }

        void ManagementAgentMain::loadPlugins()
        {
            // Load known plugins.  New plugins will be loaded via PluginServerCallback
            std::vector<Common::PluginRegistryImpl::PluginInfo> plugins =
                Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

            for (auto& plugin : plugins)
            {
                m_pluginManager->registerAndSetAppIds(
                    plugin.getPluginName(), plugin.getPolicyAppIds(), plugin.getStatusAppIds());
                LOGINFO(
                    "Registered plugin " << plugin.getPluginName() << ", executable path "
                                         << plugin.getExecutableFullPath());
            }
        }

        void ManagementAgentMain::initialiseTaskQueue()
        {
            m_taskQueue = std::make_shared<TaskQueueImpl::TaskQueueImpl>();
            m_taskQueueProcessor =
                std::unique_ptr<TaskQueue::ITaskProcessor>(new TaskQueueImpl::TaskProcessorImpl(m_taskQueue));
        }

        void ManagementAgentMain::initialiseDirectoryWatcher()
        {
            m_policyListener = std::unique_ptr<McsRouterPluginCommunicationImpl::TaskDirectoryListener>(
                new McsRouterPluginCommunicationImpl::TaskDirectoryListener(
                    ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath(),
                    m_taskQueue,
                    *m_pluginManager));
            m_actionListener = std::unique_ptr<McsRouterPluginCommunicationImpl::TaskDirectoryListener>(
                new McsRouterPluginCommunicationImpl::TaskDirectoryListener(
                    ApplicationConfiguration::applicationPathManager().getMcsActionFilePath(),
                    m_taskQueue,
                    *m_pluginManager));

            m_directoryWatcher =
                std::unique_ptr<DirectoryWatcher::IDirectoryWatcher>(new DirectoryWatcherImpl::DirectoryWatcher());
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

        void ManagementAgentMain::sendCurrentPluginsStatus(const std::vector<std::string>& registeredPlugins)
        {
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
                    std::unique_ptr<Common::TaskQueue::ITask> task(new StatusReceiverImpl::StatusTask(
                        m_statusCache,
                        pluginStatusInfo.appId,
                        pluginStatusInfo.statusXml,
                        pluginStatusInfo.statusWithoutTimestampsXml,
                        tempDir,
                        statusDir));

                    m_taskQueue->queueTask(task);
                }
            }
        }

        void ManagementAgentMain::sendCurrentPluginPolicies()
        {
            std::string mcsDir = ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath();

            auto policies = Common::FileSystem::fileSystem()->listFiles(mcsDir);
            for (auto& filePath : policies)
            {
                std::unique_ptr<Common::TaskQueue::ITask> task(
                    new McsRouterPluginCommunicationImpl::PolicyTask(*m_pluginManager, filePath));
                m_taskQueue->queueTask(task);
            }
        }

        void ManagementAgentMain::sendCurrentActions()
        {
            std::string actionDir = ApplicationConfiguration::applicationPathManager().getMcsActionFilePath();

            auto actions = Common::FileSystem::fileSystem()->listFiles(actionDir);
            for (auto& filePath : actions)
            {
                Common::TaskQueue::ITaskPtr task(
                    new McsRouterPluginCommunicationImpl::ActionTask(*m_pluginManager, filePath));
                m_taskQueue->queueTask(task);
            }
        }

        int ManagementAgentMain::run()
        {
            LOGINFO("Management Agent starting.. ");

            // Setup SIGNAL handling for shutdown.
            Common::ZeroMQWrapper::IHasFDPtr shutdownPipePtr;
            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

            GL_signalPipe = std::unique_ptr<Common::Threads::NotifyPipe>(new Common::Threads::NotifyPipe());
            struct sigaction action; // NOLINT
            action.sa_handler = s_signal_handler;
            action.sa_flags = 0;
            sigemptyset(&action.sa_mask);
            sigaction(SIGINT, &action, nullptr);
            sigaction(SIGTERM, &action, nullptr);

            shutdownPipePtr = poller->addEntry(GL_signalPipe->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

            // start running background threads
            m_taskQueueProcessor->start();
            m_directoryWatcher->startWatch();

            LOGINFO("Management Agent running.");

            bool running = true;
            while (running)
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

        void ManagementAgentMain::test_request_stop() { GL_signalPipe->notify(); }

    } // namespace ManagementAgentImpl
} // namespace ManagementAgent
