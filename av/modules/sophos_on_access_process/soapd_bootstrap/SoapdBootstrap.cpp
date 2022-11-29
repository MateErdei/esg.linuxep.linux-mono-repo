// Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
#include "OnAccessProcesControlCallbacks.h"
#include "OnAccessProductConfigDefaults.h"
#include "OnAccessServiceCallback.h"
// Component
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
// Product
#include "common/FDUtils.h"
#include "common/PidLockFile.h"
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "datatypes/SystemCallWrapper.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

// Base
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
// Std C++
#include <filesystem>
#include <fstream>
#include <memory>
// Std C
#include <poll.h>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;
using namespace sophos_on_access_process::service_callback;
using namespace unixsocket::updateCompleteSocket;


int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");
    auto SoapdInstance = SoapdBootstrap();
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


void SoapdBootstrap::initialiseTelemetry()
{
    m_TelemetryUtility = std::make_shared<onaccesstelemetry::OnAccessTelemetryUtility>();

    Common::Telemetry::TelemetryHelper::getInstance().restore(OnAccessConfig::onAccessTelemetrySocket);
    auto replier = m_onAccessContext->getReplier();
    Common::PluginApiImpl::PluginResourceManagement::setupReplier(*replier, OnAccessConfig::onAccessTelemetrySocket, 5000, 5000);
    auto pluginCallback = std::make_shared<sophos_on_access_process::service_callback::OnAccessServiceCallback>(m_TelemetryUtility);
    m_pluginHandler = std::make_unique<Common::PluginApiImpl::PluginCallBackHandler>
        (OnAccessConfig::onAccessTelemetrySocket,
         std::move(replier),
        pluginCallback,
        Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::DONOTARM);

    m_pluginHandler->start();
}

void SoapdBootstrap::innerRun()
{
    // Take soapd lock file
    fs::path lockfile = common::getPluginInstallPath() / "var/soapd.pid";
    common::PidLockFile lock(lockfile);

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    OnAccessConfig::OnAccessConfiguration config;

    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();

    size_t maxScanQueueSize = 0;
    OnAccessConfig::readProductConfigFile(maxScanQueueSize, m_maxNumberOfScanThreads, m_dumpPerfData);

    const struct rlimit file_lim = { onAccessProcessFdLimit, onAccessProcessFdLimit };
    sysCallWrapper->setrlimit(RLIMIT_NOFILE, &file_lim);

    m_fanotifyHandler = std::make_shared<FanotifyHandler>(sysCallWrapper);

    auto sysPathsFactory = std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>();
    m_mountMonitor = std::make_shared<mount_monitor::mount_monitor::MountMonitor>(config,
                                                                                  sysCallWrapper,
                                                                                  m_fanotifyHandler,
                                                                                  sysPathsFactory);
    m_mountMonitorThread = std::make_unique<common::ThreadRunner>(m_mountMonitor,
                                                                  "mountMonitor",
                                                                  false);

    fs::path updateSocketPath = common::getPluginInstallPath() / "chroot/var/update_complete_socket";
    auto updateClient = std::make_shared<UpdateCompleteClientSocketThread>(updateSocketPath, m_fanotifyHandler);
    auto updateClientThread = std::make_unique<common::ThreadRunner>(updateClient, "updateClient", true);

    m_scanRequestQueue = std::make_shared<ScanRequestQueue>(maxScanQueueSize);

    initialiseTelemetry(); //This initialises m_TelemetryUtility
    m_eventReader = std::make_shared<EventReaderThread>(m_fanotifyHandler,
                                                           sysCallWrapper,
                                                           common::getPluginInstallPath(),
                                                           m_scanRequestQueue,
                                                           m_TelemetryUtility);
    m_eventReaderThread = std::make_unique<common::ThreadRunner>(m_eventReader,
                                                                 "eventReader",
                                                                 false);

    m_deviceUtil = std::make_shared<mount_monitor::mountinfoimpl::DeviceUtil>(sysCallWrapper);


    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGDEBUG("Control Server Socket is at: " << socketPath);
    auto processControlCallbacks = std::make_shared<sophos_on_access_process::OnAccessConfig::OnAccessProcessControlCallback>(*this);
    auto processControllerServer = std::make_shared<unixsocket::ProcessControllerServerSocket>(socketPath,
                                                                                               0600,
                                                                                               processControlCallbacks);
    processControllerServer->setUserAndGroup("sophos-spl-av", "sophos-spl-group");
    auto processControllerServerThread = std::make_unique<common::ThreadRunner>(processControllerServer,
                                                                                "processControlServer",
                                                                                true);

    struct pollfd fds[] {
        { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 }
    };

    auto flagJsonString = OnAccessConfig::readFlagConfigFile();
    //if flagJsonString is empty then readFlagConfigFile has already logged a WARN
    if(!flagJsonString.empty())
    {
        std::string setting = OnAccessConfig::parseFlagConfiguration(flagJsonString) ? "not override policy" : "override policy";
        LOGINFO("Flag is set to " << setting);
    }

    ProcessPolicy();

    while (true)
    {
        // wait for an activity on one of the fds
        int activity = ::ppoll(fds, std::size(fds), nullptr, nullptr);
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
    }

    // Stop the other things that can call ProcessPolicy()
    processControllerServer->tryStop();
    updateClient->tryStop();
    processControllerServerThread.reset();
    updateClientThread.reset();

    disableOnAccess();
}

sophos_on_access_process::OnAccessConfig::OnAccessConfiguration SoapdBootstrap::getPolicyConfiguration()
{
    auto jsonString = OnAccessConfig::readPolicyConfigFile();
    OnAccessConfig::OnAccessConfiguration oaConfig = OnAccessConfig::parseOnAccessPolicySettingsFromJson(jsonString);
    return oaConfig;
}

void SoapdBootstrap::ProcessPolicy()
{
    std::lock_guard<std::mutex> guard(m_pendingConfigActionMutex);
    LOGDEBUG("ProcessPolicy");

    auto flagJsonString = OnAccessConfig::readFlagConfigFile();
    auto OnAccessEnabledFlag = OnAccessConfig::parseFlagConfiguration(flagJsonString);

    auto oaConfig = getPolicyConfiguration();
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

void SoapdBootstrap::disableOnAccess()
{
    // Log if on-access was enabled before
    if (!m_currentOaEnabledState)
    {
        return;
    }

    LOGINFO("Disabling on-access scanning");
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

    m_fanotifyHandler->init();

    // Ensure queue is empty and running before we put anything in to it.
    m_scanRequestQueue->restart();

    // Start reading for events before we mark filesystems.
    m_eventReaderThread->startIfNotStarted();

    m_mountMonitorThread->startIfNotStarted(); // Returns only once initial marks are done

    std::string scanRequestSocketPath = common::getPluginInstallPath() / "chroot/var/scanning_socket";

    for (int threadCount = 0; threadCount < m_maxNumberOfScanThreads; ++threadCount)
    {
        std::stringstream threadName;
        threadName << "scanHandler " << threadCount;

        auto scanningSocket = std::make_shared<unixsocket::ScanningClientSocket>(scanRequestSocketPath);
        auto scanHandler = std::make_shared<ScanRequestHandler>(
            m_scanRequestQueue, scanningSocket, m_fanotifyHandler, m_deviceUtil, m_TelemetryUtility, threadCount, m_dumpPerfData);
        auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, threadName.str(), true);
        m_scanHandlerThreads.push_back(scanHandlerThread);
    }

    LOGINFO("On-access scanning enabled");
    m_currentOaEnabledState.store(true);
}
