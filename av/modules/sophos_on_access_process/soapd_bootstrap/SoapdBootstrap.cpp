// Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
// Component
#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigMonitor.h"
#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"
// Product
#include "common/FDUtils.h"
#include "common/PidLockFile.h"
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
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

    // Take soapd lock file
    fs::path lockfile = common::getPluginInstallPath() / "var/soapd.pid";
    common::PidLockFile lock(lockfile);

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGINFO("Socket is at: " << socketPath);

    Common::Threads::NotifyPipe onAccessConfigPipe;
    Common::Threads::NotifyPipe usePolicyOverridePipe;
    Common::Threads::NotifyPipe useFlagOverridePipe;
    auto configMonitor = std::make_shared<OnAccessConfig::OnAccessConfigMonitor>(socketPath,
                                                                         onAccessConfigPipe,
                                                                         usePolicyOverridePipe,
                                                                         useFlagOverridePipe);
    auto configMonitorThread = std::make_unique<common::ThreadRunner>(configMonitor, "configMonitor", true);

    try
    {
        innerRun(sigIntMonitor,
                 sigTermMonitor,
                 onAccessConfigPipe,
                 usePolicyOverridePipe,
                 useFlagOverridePipe);
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
    std::shared_ptr<common::signals::SigTermMonitor>& sigTermMonitor,
    Common::Threads::NotifyPipe onAccessConfigPipe,
    Common::Threads::NotifyPipe usePolicyOverridePipe,
    Common::Threads::NotifyPipe useFlagOverridePipe)
{
    mount_monitor::mount_monitor::OnAccessMountConfig config;
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>();
    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
    auto fanotifyHandler = std::make_unique<FanotifyHandler>(sysCallWrapper);
    auto mountMonitor = std::make_shared<mount_monitor::mount_monitor::MountMonitor>(config, sysCallWrapper, fanotifyHandler->getFd());
    auto mountMonitorThread = std::make_unique<common::ThreadRunner>(mountMonitor, "mountMonitor", true);
    auto eventReader = std::make_shared<EventReaderThread>(fanotifyHandler->getFd(), sysCallWrapper, common::getPluginInstallPath(), scanRequestQueue);
    auto eventReaderThread = std::make_unique<common::ThreadRunner>(eventReader, "eventReader", false);

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
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = onAccessConfigPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = usePolicyOverridePipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = useFlagOverridePipe.readFd(), .events = POLLIN, .revents = 0 },
    };

    bool currentOaEnabledState = false;
    bool policyOverride = true;
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

        if ((fds[2].revents & POLLIN) != 0)
        {
            onAccessConfigPipe.notified();

            if(policyOverride)
            {
                LOGINFO("Policy override is enabled, on-access will be disabled");
            }
            else
            {
                auto oaConfig = getConfiguration(mountMonitor);

                bool oldOaEnabledState = currentOaEnabledState;
                currentOaEnabledState = oaConfig.enabled;
                if (currentOaEnabledState && !oldOaEnabledState)
                {
                    LOGINFO("On-access scanning enabled");
                    eventReaderThread->startIfNotStarted();
                }
                if (!currentOaEnabledState && oldOaEnabledState)
                {
                    LOGINFO("On-access scanning disabled");
                    eventReaderThread->requestStopIfNotStopped();
                }
            }
        }

        if ((fds[3].revents & POLLIN) != 0)
        {
            usePolicyOverridePipe.notified();

            LOGINFO("Using on-access settings from policy");
            policyOverride = false;
            auto oaConfig = getConfiguration(mountMonitor);
            if(oaConfig.enabled)
            {
                LOGINFO("On-access scanning enabled");
                eventReaderThread->startIfNotStarted();
            }
        }

        if ((fds[4].revents & POLLIN) != 0)
        {
            useFlagOverridePipe.notified();
            LOGINFO("Overriding policy and disabling on-access");
            policyOverride = true;
            eventReaderThread->requestStopIfNotStopped();
        }
    }

    scanRequestQueue->stop();
}

sophos_on_access_process::OnAccessConfig::OnAccessConfiguration SoapdBootstrap::getConfiguration(
    std::shared_ptr<mount_monitor::mount_monitor::MountMonitor>& mountMonitor)
{
    auto jsonString = OnAccessConfig::readConfigFile();
    OnAccessConfig::OnAccessConfiguration oaConfig = OnAccessConfig::parseOnAccessSettingsFromJson(jsonString);
    mountMonitor->setExcludeRemoteFiles(oaConfig.excludeRemoteFiles);

    return oaConfig;
}
