/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ManagementAgentMain.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/PluginCommunication/IPluginCommunicationException.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/ConfigException.h>
#include <Common/ZeroMQWrapper/IHasFD.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapper/IProxy.h>
#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/HealthTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <sys/stat.h>

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

    std::string sophosManagementPluginName("sophos_managementagent");

} // namespace

namespace ManagementAgent
{
    namespace ManagementAgentImpl
    {
        int ManagementAgentMain::main(int argc, char** argv)
        {
            umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
            static_cast<void>(argv);            // unused
            Common::Logging::FileLoggingSetup loggerSetup(sophosManagementPluginName, true);
            if (argc > 1)
            {
                LOGERROR("Error, invalid command line arguments. Usage: Management Agent");
                return -1;
            }
            return mainForValidArguments(); 
        }
        
        int ManagementAgentMain::mainForValidArguments(bool withPersistentTelemetry)
        {
            try
            {
                std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager =
                    std::unique_ptr<ManagementAgent::PluginCommunication::IPluginManager>(
                        new ManagementAgent::PluginCommunicationImpl::PluginManager());

                ManagementAgentMain managementAgent;
                managementAgent.initialise(*pluginManager);
                return managementAgent.run(withPersistentTelemetry);
            }
            catch (Common::UtilityImpl::ConfigException& ex)
            {
                LOGFATAL(ex.what());
                return 1;
            }
        }

        void ManagementAgentMain::initialise(ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
        {
            LOGINFO("Initializing Management Agent");

            m_pluginManager = &pluginManager;
            m_statusCache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
            try
            {
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
                m_healthStatus = std::make_shared<ManagementAgent::McsRouterPluginCommunicationImpl::HealthStatus>();

                m_ppid = ::getppid();
            }catch ( std::exception & ex)
            {
                throw Common::UtilityImpl::ConfigException( "Configure Management Agent", ex.what());
            }

        }

        void ManagementAgentMain::loadPlugins()
        {
            // Load known plugins.  New plugins will be loaded via PluginServerCallback
            std::vector<Common::PluginRegistryImpl::PluginInfo> plugins =
                Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

            for (auto& plugin : plugins)
            {
                if(plugin.getIsManagedPlugin())
                {
                    m_pluginManager->registerAndConfigure(
                            plugin.getPluginName(), PluginCommunication::PluginDetails(plugin));
                    LOGINFO("Registered plugin " << plugin.getPluginName() << ", executable path "
                        << plugin.getExecutableFullPath());
                }

                if(plugin.getHasThreatServiceHealth())
                {
                    LOGDEBUG("Registered plugin " << plugin.getPluginName() << ", has threat health enabled");
                }

                if(plugin.getHasServiceHealth())
                {
                    LOGDEBUG("Registered plugin " << plugin.getPluginName() << ", has service health enabled");
                }
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

            m_directoryWatcher = Common::DirectoryWatcher::createDirectoryWatcher();
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
                catch (Common::PluginCommunication::IPluginCommunicationException& e)
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

                    m_taskQueue->queueTask(std::move(task));
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
                m_taskQueue->queueTask(std::move(task));
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
                m_taskQueue->queueTask(std::move(task));
            }
        }

        int ManagementAgentMain::run(bool withPersistentTelemetry)
        {
            LOGINFO("Management Agent starting.. ");

            // Restore telemetry from disk
            if (withPersistentTelemetry)
            {
                Common::Telemetry::TelemetryHelper::getInstance().restore(sophosManagementPluginName);
            }

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
            auto lastHealthCheck = std::chrono::system_clock::now();
            const int waitPeriod = 15; // Should not exceed health refresh period of 15 seconds.

            while (running)
            {
                auto currentTime = std::chrono::system_clock::now();
                if ((currentTime - lastHealthCheck) >= std::chrono::seconds (waitPeriod))
                {
                    lastHealthCheck = currentTime;
                    std::unique_ptr<Common::TaskQueue::ITask> task(new McsRouterPluginCommunicationImpl::HealthTask(*m_pluginManager, m_healthStatus));
                    m_taskQueue->queueTask(std::move(task));
                }

                poller->poll(std::chrono::seconds(waitPeriod));
                if (GL_signalPipe->notified())
                {
                    LOGDEBUG("Management Agent stopping");
                    running = false;
                }
                if (::getppid() != m_ppid)
                {
                    LOGWARN("Management Agent stopping because parent process has changed");
                    running = false;
                }
            }

            // pre-pare and stop back ground threads
            m_directoryWatcher->removeListener(*m_policyListener);
            m_directoryWatcher->removeListener(*m_actionListener);
            m_taskQueueProcessor->stop();

            if (withPersistentTelemetry)
            {
                //save telemetry to disk
                Common::Telemetry::TelemetryHelper::getInstance().save();
            }

            LOGDEBUG("Management Agent stopped");
            return 0;
        }

        void ManagementAgentMain::test_request_stop() { GL_signalPipe->notify(); }

    } // namespace ManagementAgentImpl
} // namespace ManagementAgent
