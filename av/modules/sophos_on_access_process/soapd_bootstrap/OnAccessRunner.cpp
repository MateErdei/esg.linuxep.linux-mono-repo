// Copyright 2023, Sophos Limited.  All rights reserved.

#include "OnAccessRunner.h"

// Package
#include "Logger.h"
#include "ProcessPriorityUtils.h"
// Component
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
// Product
#include "common/PluginUtils.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
//C++
#include <cassert>

using namespace sophos_on_access_process::soapd_bootstrap;

using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;
using namespace sophos_on_access_process::OnAccessConfig;

void OnAccessRunner::setupOnAccess()
{
    assert(!m_fanotifyHandler); // setupOnAccess() should only be called once.

    m_localSettings = OnAccessConfig::readLocalSettingsFile(m_sysCallWrapper);

    const struct rlimit file_lim = { onAccessProcessFdLimit, onAccessProcessFdLimit };
    m_sysCallWrapper->setrlimit(RLIMIT_NOFILE, &file_lim);

    m_fanotifyHandler = std::make_shared<FanotifyHandler>(m_sysCallWrapper);

    auto sysPathsFactory = std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>();
    m_mountMonitor = std::make_shared<mount_monitor::mount_monitor::MountMonitor>(m_config,
                                                                                  m_sysCallWrapper,
                                                                                  m_fanotifyHandler,
                                                                                  sysPathsFactory);
    m_mountMonitorThread = std::make_unique<common::ThreadRunner>(m_mountMonitor,
                                                                  "mountMonitor",
                                                                  false);

    size_t maxScanQueueSize = m_localSettings.maxScanQueueSize;
    m_scanRequestQueue = std::make_shared<ScanRequestQueue>(maxScanQueueSize);

    m_deviceUtil = std::make_shared<mount_monitor::mountinfoimpl::DeviceUtil>(m_sysCallWrapper);

    m_eventReader = std::make_shared<EventReaderThread>(m_fanotifyHandler,
                                                        m_sysCallWrapper,
                                                        common::getPluginInstallPath(),
                                                        m_scanRequestQueue,
                                                        m_TelemetryUtility,
                                                        m_deviceUtil);
    m_eventReader->setCacheAllEvents(m_localSettings.cacheAllEvents);
    m_eventReaderThread = std::make_unique<common::ThreadRunner>(m_eventReader,
                                                                 "eventReader",
                                                                 false);
}

void OnAccessRunner::disableOnAccess()
{
    // Log if on-access was enabled before
    if (!m_currentOaEnabledState)
    {
        return;
    }

    LOGINFO("Disabling on-access scanning");

    assert(m_fanotifyHandler); // should have been set by setupOnAccess();
    m_statusFile.disabled();

    /*
     * EventReader and scanHandler threads are not thread-safe with
     * fanotify handler being closed.
     *
     * mount monitor should be thread-safe vs. fanotify handler being closed
     * but isn't worth running when fanotify is turned off.
     */

    // Async stop event reader and mount monitor
    m_eventReader->tryStop();
    m_mountMonitor->tryStop();

    // sync stop event reader and mount monitor
    m_eventReaderThread->requestStopIfNotStopped();
    m_mountMonitorThread->requestStopIfNotStopped();

    // stop scan threads, and clear queue
    m_scanRequestQueue->stop(); // Clears queue

    // Join all scan threads
    m_scanHandlerThreads.clear();

    // Disable fanotify and close fanotify descriptor
    m_fanotifyHandler->close();

    // Set priority
    if (m_localSettings.highPrioritySoapd)
    {
        m_sysCallWrapper->setpriority(PRIO_PROCESS, 0, 10);
    }

    if (m_localSettings.highPriorityThreatDetector)
    {
        setThreatDetectorPriority(0, m_sysCallWrapper); // Return to default priority
    }

    LOGINFO("On-access scanning disabled");
    m_currentOaEnabledState.store(false);
}

void OnAccessRunner::enableOnAccess()
{
    if (m_currentOaEnabledState)
    {
        return;
    }

    LOGINFO("Enabling on-access scanning");

    assert(m_fanotifyHandler); // should have been set by setupOnAccess();
    m_fanotifyHandler->init();

    // Ensure queue is empty and running before we put anything in to it.
    m_scanRequestQueue->restart();

    // Start reading for events before we mark filesystems.
    m_eventReaderThread->startIfNotStarted();

    m_mountMonitorThread->startIfNotStarted(); // Returns only once initial marks are done

    std::string scanRequestSocketPath = common::getPluginInstallPath() / "chroot/var/scanning_socket";

    for (int threadCount = 0; threadCount < m_localSettings.numScanThreads; ++threadCount)
    {
        std::stringstream threadName;
        threadName << "scanHandler " << threadCount;

        auto scanningSocket = std::make_shared<unixsocket::ScanningClientSocket>(scanRequestSocketPath);
        auto scanHandler = std::make_shared<ScanRequestHandler>(
            m_scanRequestQueue,
            scanningSocket,
            m_fanotifyHandler,
            m_deviceUtil,
            m_TelemetryUtility,
            threadCount,
            m_localSettings);
        auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, threadName.str(), true);
        m_scanHandlerThreads.push_back(scanHandlerThread);
    }

    // Set priority
    if (m_localSettings.highPrioritySoapd)
    {
        m_sysCallWrapper->setpriority(PRIO_PROCESS, 0, -20);
    }

    if (m_localSettings.highPriorityThreatDetector)
    {
        setThreatDetectorPriority(-20, m_sysCallWrapper); // Set TD to high priority
    }

    m_statusFile.enabled();
    LOGINFO("On-access scanning enabled");
    m_currentOaEnabledState.store(true);
}

void OnAccessRunner::applyConfig(const OnAccessConfiguration& config)
{
    m_eventReader->setDetectPUAs(config.detectPUAs);
    m_eventReader->setExclusions(config.exclusions);
    m_mountMonitor->updateConfig(config);
}

bool OnAccessRunner::getPolicyConfiguration(OnAccessConfiguration& oaConfig)
{
    auto jsonString = OnAccessConfig::readPolicyConfigFile();
    return OnAccessConfig::parseOnAccessPolicySettingsFromJson(jsonString, oaConfig);
}

void OnAccessRunner::ProcessPolicy()
{
    std::lock_guard<std::mutex> guard(m_pendingConfigActionMutex);
    LOGDEBUG("ProcessPolicy");

    OnAccessConfig::OnAccessConfiguration oaConfig{};

    if (getPolicyConfiguration(oaConfig))
    {
        bool OnAccessEnabledPolicySetting = oaConfig.enabled;

        if(OnAccessEnabledPolicySetting)
        {
            //Set exclusions first before starting receiving fanotify events
            applyConfig(oaConfig);
            enableOnAccess();
        }
        else
        {
            disableOnAccess();
        }
        LOGDEBUG("Finished ProcessPolicy");
    }
    //Logging for failure done in calls from getPolicyConfiguration
}


timespec* OnAccessRunner::getTimeout()
{
    static timespec timeout { .tv_sec=60, .tv_nsec=0 };

    if (m_currentOaEnabledState && m_localSettings.highPriorityThreatDetector)
    {
        return &timeout;
    }
    return nullptr;
}

void OnAccessRunner::onTimeout()
{
    if (m_currentOaEnabledState && m_localSettings.highPriorityThreatDetector)
    {
        // Reset priority in case TD has restarted
        setThreatDetectorPriority(-20, m_sysCallWrapper); // Set TD to high priority
    }
}

