// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ManagementAgentMain.h"

#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/PluginCommunication/IPluginCommunicationException.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "Common/TaskQueueImpl/TaskProcessorImpl.h"
#include "Common/TaskQueueImpl/TaskQueueImpl.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/ConfigException.h"
#include "Common/UtilityImpl/SystemExecutableUtils.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/IPoller.h"
#include "ManagementAgent/EventReceiverImpl/EventReceiverImpl.h"
#include "ManagementAgent/HealthTask/HealthTask.h"
#include "ManagementAgent/LoggerImpl/Logger.h"
#include "ManagementAgent/McsRouterPluginCommunicationImpl/ActionTask.h"
#include "ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h"
#include "ManagementAgent/PluginCommunicationImpl/PluginManager.h"
#include "ManagementAgent/StatusCacheImpl/StatusCache.h"
#include "ManagementAgent/StatusReceiverImpl/StatusTask.h"
#include "ManagementAgent/ThreatHealthReceiverImpl/ThreatHealthReceiverImpl.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

#include <sys/stat.h>

#include <csignal>
#include <memory>

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

namespace ManagementAgent::ManagementAgentImpl
{
    int ManagementAgentMain::main(int argc, char** argv)
    {
        umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
        static_cast<void>(argv);            // unused
        Common::Logging::FileLoggingSetup loggerSetup(sophosManagementPluginName, true);
        LOGINFO("Management Agent " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");
        if (argc > 1)
        {
            LOGERROR("Error, invalid command line arguments. Usage: Management Agent");
            return -1;
        }
        return mainForValidArguments();
    }

    int ManagementAgentMain::mainForValidArguments(bool withPersistentTelemetry)
    {
        int ret = 2;
        try
        {
            {
                auto pluginManager = std::make_shared<ManagementAgent::PluginCommunicationImpl::PluginManager>(
                    Common::ZMQWrapperApi::createContext());

                ManagementAgentMain managementAgent;
                managementAgent.initialise(std::move(pluginManager));
                ret = managementAgent.run(withPersistentTelemetry);
            }
            LOGINFO("Management Agent stopped"); // Actually logged after all of the objects are destroyed
        }
        catch (const Common::UtilityImpl::ConfigException& ex)
        {
            LOGFATAL(ex.what_with_location());
            ret = 1;
        }
        catch (const Common::Exceptions::IException& ex)
        {
            LOGFATAL(ex.what_with_location());
            ret = 1;
        }
        catch (const std::exception& ex)
        {
            LOGFATAL(ex.what());
            ret = 1;
        }
        return ret;
    }

    void ManagementAgentMain::initialise(PluginManagerPtr pluginManager)
    {
        LOGINFO("Initializing Management Agent");

        m_pluginManager = std::move(pluginManager);
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
            m_ppid = ::getppid();
        }
        catch (std::exception& ex)
        {
            std::throw_with_nested(Common::UtilityImpl::ConfigException("Configure Management Agent", ex.what()));
        }
    }

    void ManagementAgentMain::loadPlugins()
    {
        // Load known plugins.  New plugins will be loaded via PluginServerCallback
        std::vector<Common::PluginRegistryImpl::PluginInfo> plugins =
            Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

        for (auto& plugin : plugins)
        {
            if (plugin.getIsManagedPlugin())
            {
                m_pluginManager->registerAndConfigure(
                    plugin.getPluginName(), PluginCommunication::PluginDetails(plugin));
                LOGINFO(
                    "Registered plugin " << plugin.getPluginName() << ", executable path "
                                         << plugin.getExecutableFullPath());
            }

            if (plugin.getHasThreatServiceHealth())
            {
                LOGDEBUG("Registered plugin " << plugin.getPluginName() << ", has threat health enabled");
            }

            if (plugin.getHasServiceHealth())
            {
                LOGDEBUG("Registered plugin " << plugin.getPluginName() << ", has service health enabled");
            }
        }
    }

    void ManagementAgentMain::initialiseTaskQueue()
    {
        m_taskQueue = std::make_shared<TaskQueueImpl::TaskQueueImpl>();
        m_taskQueueProcessor = std::make_unique<TaskQueueImpl::TaskProcessorImpl>(m_taskQueue);
    }

    void ManagementAgentMain::initialiseDirectoryWatcher()
    {
        m_policyListener = std::make_unique<McsRouterPluginCommunicationImpl::TaskDirectoryListener>(
            ApplicationConfiguration::applicationPathManager().getMcsPolicyFilePath(), m_taskQueue, *m_pluginManager);
        m_internalPolicyListener = std::make_unique<McsRouterPluginCommunicationImpl::TaskDirectoryListener>(
            ApplicationConfiguration::applicationPathManager().getInternalPolicyFilePath(),
            m_taskQueue,
            *m_pluginManager);
        m_actionListener = std::make_unique<McsRouterPluginCommunicationImpl::TaskDirectoryListener>(
            ApplicationConfiguration::applicationPathManager().getMcsActionFilePath(), m_taskQueue, *m_pluginManager);

        m_directoryWatcher = Common::DirectoryWatcher::createDirectoryWatcher();
        m_directoryWatcher->addListener(*m_policyListener);
        m_directoryWatcher->addListener(*m_internalPolicyListener);
        m_directoryWatcher->addListener(*m_actionListener);
    }

    void ManagementAgentMain::initialisePluginReceivers()
    {
        m_policyReceiver = std::make_shared<PolicyReceiverImpl::PolicyReceiverImpl>(m_taskQueue, *m_pluginManager);
        m_statusReceiver = std::make_shared<StatusReceiverImpl::StatusReceiverImpl>(m_taskQueue, m_statusCache);
        m_eventReceiver = std::make_shared<EventReceiverImpl::EventReceiverImpl>(m_taskQueue);
        m_threatHealthReceiver =
            std::make_shared<ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl>(m_taskQueue);

        m_pluginManager->setPolicyReceiver(m_policyReceiver);
        m_pluginManager->setStatusReceiver(m_statusReceiver);
        m_pluginManager->setEventReceiver(m_eventReceiver);
        m_pluginManager->setThreatHealthReceiver(m_threatHealthReceiver);
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
                LOGWARN("Failed to get plugin status for: " << pluginName << ", with error: " << e.what());
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

    void ManagementAgentMain::ensureOverallHealthFileExists()
    {
        std::string filePath = ApplicationConfiguration::applicationPathManager().getOverallHealthFilePath();
        std::string contents = "{\"health\":1,\"service\":1,\"threat\":1,\"threatService\":1}";
        std::string tempDir = ApplicationConfiguration::applicationPathManager().getTempPath();
        ;

        auto fs = Common::FileSystem::fileSystem();
        try
        {
            if (!fs->isFile(filePath))
            {
                fs->writeFileAtomically(filePath, contents, tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to create overallHealth file with error: " << ex.what());
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

        GL_signalPipe = std::make_unique<Common::Threads::NotifyPipe>();
        struct sigaction action
        {
        };
        action.sa_handler = s_signal_handler;
        action.sa_flags = SA_RESTART;
        sigemptyset(&action.sa_mask);
        sigaction(SIGINT, &action, nullptr);
        sigaction(SIGTERM, &action, nullptr);

        shutdownPipePtr = poller->addEntry(GL_signalPipe->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);
        ensureOverallHealthFileExists();
        // start running background threads
        m_taskQueueProcessor->start();
        m_directoryWatcher->startWatch();

        LOGINFO("Management Agent running.");

        bool running = true;
        auto startTime = std::chrono::steady_clock::now();
        auto lastHealthCheck = startTime;
        const auto waitPeriod = std::chrono::seconds{ 15 }; // Should not exceed health refresh period of 15 seconds.
        bool servicesShouldHaveStarted = false;
        while (running)
        {
            auto currentTime = std::chrono::steady_clock::now();
            bool checkHealth = true;
            if (m_pluginManager->updateOngoingWithGracePeriod(120, currentTime))
            {
                // Ignore everything during grace period
                // Need to do this first since it stores whether we are in an update
                checkHealth = false;
            }
            else if (!servicesShouldHaveStarted)
            {
                // always wait 60 seconds after starting for services to start, even if not during update.
                if ((currentTime - startTime) >= std::chrono::seconds(60))
                {
                    servicesShouldHaveStarted = true; // So that we only log once
                    checkHealth = true;
                    LOGINFO("Starting service health checks");
                }
                else
                {
                    // During startup we give a grace period for other processes to start up
                    checkHealth = false;
                }
            }

            if (checkHealth && (currentTime - lastHealthCheck) >= waitPeriod)
            {
                lastHealthCheck = currentTime;
                auto task = std::make_unique<HealthStatusImpl::HealthTask>(*m_pluginManager);
                m_taskQueue->queueTask(std::move(task));
            }

            poller->poll(waitPeriod);
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

        // prepare and stop background threads
        LOGDEBUG("Stopping Directory watcher");
        m_directoryWatcher->removeListener(*m_policyListener);
        m_directoryWatcher->removeListener(*m_internalPolicyListener);
        m_directoryWatcher->removeListener(*m_actionListener);
        m_directoryWatcher.reset(); // Stops thread
        m_policyListener.reset();
        m_internalPolicyListener.reset();
        m_actionListener.reset();
        LOGDEBUG("Stopped Directory watcher");

        m_policyReceiver.reset();
        m_statusReceiver.reset();
        m_eventReceiver.reset();
        m_threatHealthReceiver.reset();

        LOGDEBUG("Stopping Task Queue Processor");
        m_taskQueueProcessor->stop(); // Stops thread
        m_taskQueueProcessor.reset(); // Will stop thread if stop does not
        LOGDEBUG("Stopped Task Queue Processor");

        m_taskQueue.reset();
        m_statusCache.reset();
        m_pluginManager.reset();

        if (withPersistentTelemetry)
        {
            // save telemetry to disk
            Common::Telemetry::TelemetryHelper::getInstance().save(sophosManagementPluginName);
        }

        LOGDEBUG("Management Agent finished run");
        return 0;
    }

    void ManagementAgentMain::test_request_stop()
    {
        GL_signalPipe->notify();
    }
} // namespace ManagementAgent::ManagementAgentImpl
