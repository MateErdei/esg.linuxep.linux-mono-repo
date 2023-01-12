// Copyright 2022-2023 Sophos Limited. All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
#include "OnAccessProcesControlCallbacks.h"
#include "ProcessPriorityUtils.h"
// Component
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/local_settings/OnAccessProductConfigDefaults.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
// Product
#include "common/PidLockFile.h"
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

// Std C++
#include <memory>
#include <utility>
// Std C
#include <poll.h>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;
using namespace sophos_on_access_process::OnAccessConfig;
using namespace unixsocket::updateCompleteSocket;

SoapdBootstrap::SoapdBootstrap(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
    : m_sysCallWrapper(std::move(systemCallWrapper))
{
    assert(m_sysCallWrapper);
}

int SoapdBootstrap::runSoapd(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper)
{
    LOGINFO("Sophos on access process " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");
    auto SoapdInstance = SoapdBootstrap(std::move(systemCallWrapper));
    return SoapdInstance.outerRun();
}

int SoapdBootstrap::outerRun()
{
    try
    {
        innerRun();
    }
    catch (const std::exception& e)
    {
        LOGFATAL("Exception caught at top-level: " << e.what());
        return 1;
    }
    catch (...)
    {
        LOGFATAL("Non-std::exception caught at top-level");
        return 2;
    }

    LOGINFO("Exiting soapd");
    return 0;
}

void SoapdBootstrap::innerRun()
{
    // Take soapd lock file
    fs::path lockfile = common::getPluginInstallPath() / "var/soapd.pid";
    common::PidLockFile lock(lockfile);

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    m_ServiceImpl = std::make_unique<service_impl::OnAccessServiceImpl>();
    m_TelemetryUtility = m_ServiceImpl->getTelemetryUtility();

    setupOnAccess();

    fs::path updateSocketPath = common::getPluginInstallPath() / "chroot/var/update_complete_socket";
    assert(m_fanotifyHandler); // should be set by setupOnAccess() above
    auto updateClient = std::make_shared<UpdateCompleteClientSocketThread>(updateSocketPath, m_fanotifyHandler);
    auto updateClientThread = std::make_unique<common::ThreadRunner>(updateClient, "updateClient", true);

    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGDEBUG("Control Server Socket is at: " << socketPath);
    auto processControlCallbacks = std::make_shared<OnAccessProcessControlCallback>(*this);
    auto processControllerServer = std::make_shared<unixsocket::ProcessControllerServerSocket>(socketPath,
                                                                                               0600,
                                                                                               processControlCallbacks);
    processControllerServer->setUserAndGroup("sophos-spl-av", "sophos-spl-group");
    auto processControllerServerThread = std::make_unique<common::ThreadRunner>(processControllerServer,
                                                                                "processControlServer",
                                                                                true);

    ProcessPolicy();

    struct pollfd fds[] {
        { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 }
    };
    std::timespec timeout { .tv_sec=60, .tv_nsec=0 };

    while (true)
    {
        std::timespec* timeoutPtr = nullptr;
        if (m_currentOaEnabledState && m_localSettings.highPriorityThreatDetector)
        {
            timeoutPtr = &timeout;
        }

        // wait for an activity on one of the fds
        int activity = m_sysCallWrapper->ppoll(fds, std::size(fds), timeoutPtr, nullptr);
        if (activity < 0)
        {
            // error in ppoll
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from ppoll");
                continue;
            }

            LOGERROR("Failed to read from pipe - shutting down. Error: "
                     << common::safer_strerror(error) << " (" << error << ')');
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            LOGINFO("Sophos On Access Process received SIGINT - shutting down");
            sigIntMonitor->triggered();
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            LOGINFO("Sophos On Access Process received SIGTERM - shutting down");
            sigTermMonitor->triggered();
            break;
        }

        if (m_currentOaEnabledState && m_localSettings.highPriorityThreatDetector)
        {
            // Reset priority in case TD has restarted
            setThreatDetectorPriority(-20, m_sysCallWrapper); // Set TD to high priority
        }
    }

    // Stop the other things that can call ProcessPolicy()
    processControllerServer->tryStop();
    updateClient->tryStop();
    processControllerServerThread.reset();
    updateClientThread.reset();

    disableOnAccess();
}

bool SoapdBootstrap::getPolicyConfiguration(OnAccessConfiguration& oaConfig)
{
    auto jsonString = OnAccessConfig::readPolicyConfigFile();
    return OnAccessConfig::parseOnAccessPolicySettingsFromJson(jsonString, oaConfig);
}

void SoapdBootstrap::ProcessPolicy()
{
    std::lock_guard<std::mutex> guard(m_pendingConfigActionMutex);
    LOGDEBUG("ProcessPolicy");

    auto flagJsonString = OnAccessConfig::readFlagConfigFile();
    auto OnAccessEnabledFlag = OnAccessConfig::parseFlagConfiguration(flagJsonString);
    OnAccessConfig::OnAccessConfiguration oaConfig{};

    if (getPolicyConfiguration(oaConfig))
    {
        bool OnAccessEnabledPolicySetting = oaConfig.enabled;

        if(checkIfOAShouldBeEnabled(OnAccessEnabledFlag, OnAccessEnabledPolicySetting))
        {
            m_mountMonitor->updateConfig(oaConfig);
            //Set exclusions first before starting receiving fanotify events
            m_eventReader->setExclusions(oaConfig.exclusions);
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

bool SoapdBootstrap::checkIfOAShouldBeEnabled(bool OnAccessEnabledFlag, bool OnAccessEnabledPolicySetting)
{
    if(!OnAccessEnabledFlag)
    {
        LOGINFO("Overriding policy, on-access will be disabled");
        return false;
    }
    else
    {
        LOGINFO("No policy override, following policy settings");
        return OnAccessEnabledPolicySetting;
    }
}

void SoapdBootstrap::setupOnAccess()
{
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

    assert(m_TelemetryUtility); // should be set by innerRun() before we're called
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

void SoapdBootstrap::disableOnAccess()
{
    // Log if on-access was enabled before
    if (!m_currentOaEnabledState)
    {
        return;
    }

    LOGINFO("Disabling on-access scanning");

    m_statusFile.disabled();

    /*
     * EventReader and scanHandler threads are not thread-safe with
     * fanotify handler being closed.
     *
     * mount monitor should be thread-safe vs. fanotify handler being closed
     * but isn't worth running when fanotify is turned off.
     */

    assert(m_fanotifyHandler); // should have been set by setupOnAccess()

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

void SoapdBootstrap::enableOnAccess()
{
    if (m_currentOaEnabledState)
    {
        return;
    }

    LOGINFO("Enabling on-access scanning");

    assert(m_fanotifyHandler); // should have been set by setupOnAccess()
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
            m_scanRequestQueue, scanningSocket, m_fanotifyHandler, m_deviceUtil, m_TelemetryUtility, threadCount, m_localSettings);
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
