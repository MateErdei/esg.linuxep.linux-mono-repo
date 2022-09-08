// Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
#include "OnAccessProcesControlCallbacks.h"
// Component
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"
// Product
#include "common/FDUtils.h"
#include "common/PidLockFile.h"
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
// Base
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Std C++
#include <fstream>
#include <memory>
// Std C
#include <poll.h>

#define MAX_SCAN_THREADS 1

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;


int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    auto SoapdInstance = SoapdBootstrap();

    // Take soapd lock file
    fs::path lockfile = common::getPluginInstallPath() / "var/soapd.pid";
    common::PidLockFile lock(lockfile);

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    try
    {
        SoapdInstance.innerRun(sigIntMonitor,
                 sigTermMonitor);
    }
    catch (const std::exception& e)
    {
        LOGFATAL(e.what());
        return 1;
    }

    LOGINFO("Exiting soapd");
    return 0;
}

void SoapdBootstrap::innerRun(
    std::shared_ptr<common::signals::SigIntMonitor>& sigIntMonitor,
    std::shared_ptr<common::signals::SigTermMonitor>& sigTermMonitor)
{
    mount_monitor::mount_monitor::OnAccessMountConfig config;
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>();

    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
    auto fanotifyHandler = std::make_unique<FanotifyHandler>(sysCallWrapper);

    m_mountMonitor = std::make_shared<mount_monitor::mount_monitor::MountMonitor>(config,
                                                                                     sysCallWrapper,
                                                                                     fanotifyHandler->getFd());
    auto mountMonitorThread = std::make_unique<common::ThreadRunner>(m_mountMonitor,
                                                                     "mountMonitor",
                                                                     true);

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyHandler->getFd(),
                                                           sysCallWrapper,
                                                           common::getPluginInstallPath(),
                                                           scanRequestQueue);
     m_eventReaderThread = std::make_unique<common::ThreadRunner>(eventReader,
                                                                    "eventReader",
                                                                    false);

    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGDEBUG("Control Server Socket is at: " << socketPath);
    auto processControlCallbacks = std::make_shared<sophos_on_access_process::OnAccessConfig::OnAccessProcessControlCallback>(*this);
    auto processControllerServer = std::make_shared<unixsocket::ProcessControllerServerSocket>(socketPath,
                                                                                               0666,
                                                                                               processControlCallbacks);
    auto processControllerServerThread = std::make_unique<common::ThreadRunner>(processControllerServer,
                                                                                    "eventReader",
                                                                                    true);

    std::string scanRequestSocketPath = common::getPluginInstallPath() / "chroot/var/scanning_socket";
    auto scanningSocket = std::make_shared<unixsocket::ScanningClientSocket>(scanRequestSocketPath);

    std::vector<std::shared_ptr<common::ThreadRunner>> scanHandlerThreads;
    for (int count = 0; count < MAX_SCAN_THREADS; ++count)
    {
        auto scanHandler = std::make_shared<ScanRequestHandler>(
            scanRequestQueue, scanningSocket);
        std::stringstream threadName;
        threadName << "scanHandler" << count;
        auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, threadName.str(), true);
        scanHandlerThreads.push_back(scanHandlerThread);
    }

    struct pollfd fds[] {
        { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 }
    };

    auto flagJsonString = OnAccessConfig::readFlagConfigFile();
    //if flagJsonString is empty then readFlagConfigFile has already logged a WARN
    if(!flagJsonString.empty())
    {
        LOGINFO("Found Flag config on startup");
        m_onAccessEnabledFlag = OnAccessConfig::parseFlagConfiguration(flagJsonString);
        std::string setting = m_onAccessEnabledFlag ? "not override policy" : "override policy";
        LOGINFO("Flag is set to " << setting);
    }

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

    scanRequestQueue->stop();
}

sophos_on_access_process::OnAccessConfig::OnAccessConfiguration SoapdBootstrap::getPolicyConfiguration()
{
    auto jsonString = OnAccessConfig::readPolicyConfigFile();
    OnAccessConfig::OnAccessConfiguration oaConfig = OnAccessConfig::parseOnAccessPolicySettingsFromJson(jsonString);
    m_mountMonitor->setExcludeRemoteFiles(oaConfig.excludeRemoteFiles);

    return oaConfig;
}

void SoapdBootstrap::ProcessPolicy()
{
    std::lock_guard<std::mutex> guard(m_pendingConfigActionMutex);

    auto flagJsonString = OnAccessConfig::readFlagConfigFile();
    m_onAccessEnabledFlag = OnAccessConfig::parseFlagConfiguration(flagJsonString);

    if(m_onAccessEnabledFlag)
    {
        LOGINFO("Policy override is enabled, on-access will be disabled");
        m_eventReaderThread->requestStopIfNotStopped();
    }
    else
    {
        LOGINFO("No policy override, following policy settings");

        auto oaConfig = getPolicyConfiguration();

        bool oldOaEnabledState = m_currentOaEnabledState;
        m_currentOaEnabledState = oaConfig.enabled;
        if (m_currentOaEnabledState && !oldOaEnabledState)
        {
            LOGINFO("On-access scanning enabled");
            m_eventReaderThread->startIfNotStarted();
        }
        if (!m_currentOaEnabledState && oldOaEnabledState)
        {
            LOGINFO("On-access scanning disabled");
            m_eventReaderThread->requestStopIfNotStopped();
        }
    }
}