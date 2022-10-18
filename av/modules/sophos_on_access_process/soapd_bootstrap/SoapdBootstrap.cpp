// Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
#include "OnAccessProcesControlCallbacks.h"
// Component
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
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
#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

// Base
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Std C++
#include <filesystem>
#include <fstream>
#include <memory>
// Std C
#include <poll.h>

#define MAX_SCAN_THREADS 1

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;
using namespace unixsocket::updateCompleteSocket;


int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
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
        LOGFATAL("Non-std::exeption caught at top-level");
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

    mount_monitor::mount_monitor::OnAccessMountConfig config;

    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();

    const struct rlimit file_lim = { 4096, 4096 };
    sysCallWrapper->setrlimit(RLIMIT_NOFILE, &file_lim);

    m_fanotifyHandler = std::make_shared<FanotifyHandler>(sysCallWrapper);

    m_mountMonitor = std::make_shared<mount_monitor::mount_monitor::MountMonitor>(config,
                                                                                  sysCallWrapper,
                                                                                  m_fanotifyHandler);
    m_mountMonitorThread = std::make_unique<common::ThreadRunner>(m_mountMonitor,
                                                                  "mountMonitor",
                                                                  false);

    fs::path updateSocketPath = common::getPluginInstallPath() / "chroot/var/update_complete_socket";
    auto updateClient = std::make_shared<UpdateCompleteClientSocketThread>(updateSocketPath, m_fanotifyHandler);
    auto updateClientThread = std::make_unique<common::ThreadRunner>(updateClient, "updateClient", true);

    m_scanRequestQueue = std::make_shared<ScanRequestQueue>();

    m_eventReader = std::make_shared<EventReaderThread>(m_fanotifyHandler,
                                                           sysCallWrapper,
                                                           common::getPluginInstallPath(),
                                                           m_scanRequestQueue);
    m_eventReaderThread = std::make_unique<common::ThreadRunner>(m_eventReader,
                                                                 "eventReader",
                                                                 false);

    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGDEBUG("Control Server Socket is at: " << socketPath);
    auto processControlCallbacks = std::make_shared<sophos_on_access_process::OnAccessConfig::OnAccessProcessControlCallback>(*this);
    auto processControllerServer = std::make_shared<unixsocket::ProcessControllerServerSocket>(socketPath,
                                                                                               0666,
                                                                                               processControlCallbacks);
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

    disableOnAccess();
}

sophos_on_access_process::OnAccessConfig::OnAccessConfiguration SoapdBootstrap::getPolicyConfiguration()
{
    auto jsonString = OnAccessConfig::readPolicyConfigFile();
    OnAccessConfig::OnAccessConfiguration oaConfig = OnAccessConfig::parseOnAccessPolicySettingsFromJson(jsonString);
    m_mountMonitor->updateConfig(oaConfig.exclusions, oaConfig.excludeRemoteFiles);

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
        enableOnAccess();
        m_eventReader->setExclusions(oaConfig.exclusions);
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
    if (!m_currentOaEnabledState)
    {
        LOGINFO("On-access scanning already disabled");
        return;
    }

    LOGINFO("Disabling on-access scanning");
    m_eventReaderThread->requestStopIfNotStopped();
    m_mountMonitorThread->requestStopIfNotStopped();
    m_scanRequestQueue->stop();

    m_scanHandlerThreads.clear();

    m_fanotifyHandler->close();

    LOGINFO("On-access scanning disabled");
    m_currentOaEnabledState.store(false);
}

void SoapdBootstrap::enableOnAccess()
{
    if (m_currentOaEnabledState)
    {
        LOGINFO("On Access already enabled");
        return;
    }

    LOGINFO("Enabling on-access scanning");

    m_fanotifyHandler->init();

    m_eventReaderThread->startIfNotStarted();
    m_mountMonitorThread->startIfNotStarted();

    m_scanRequestQueue->restart();

    std::string scanRequestSocketPath = common::getPluginInstallPath() / "chroot/var/scanning_socket";

    for (int count = 0; count < MAX_SCAN_THREADS; ++count)
    {
        std::stringstream threadName;
        threadName << "scanHandler " << count;

        auto scanningSocket = std::make_shared<unixsocket::ScanningClientSocket>(scanRequestSocketPath);
        auto scanHandler = std::make_shared<ScanRequestHandler>(
            m_scanRequestQueue, scanningSocket, m_fanotifyHandler);
        auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, threadName.str(), true);
        m_scanHandlerThreads.push_back(scanHandlerThread);
    }

    LOGINFO("On-access scanning enabled");
    m_currentOaEnabledState.store(true);
}
