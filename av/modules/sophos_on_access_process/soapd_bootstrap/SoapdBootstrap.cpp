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
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
// Base
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Std C++
#include <memory>
// Std C
#include <poll.h>

#define MAX_SCAN_THREADS 10

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::fanotifyhandler;

int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGINFO("Socket is at: " << socketPath);

    Common::Threads::NotifyPipe onAccessConfigPipe;
    auto configMonitor = std::make_shared<OnAccessConfig::OnAccessConfigMonitor>(socketPath, onAccessConfigPipe);
    auto configMonitorThread = std::make_unique<common::ThreadRunner>(configMonitor, "configMonitor", true);

    try
    {
        innerRun(sigIntMonitor, sigTermMonitor, onAccessConfigPipe);
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
    Common::Threads::NotifyPipe pipe)
{
    mount_monitor::mount_monitor::OnAccessMountConfig config;
    auto scanRequestQueue = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestQueue>();
    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
    auto fanotifyHandler = std::make_unique<FanotifyHandler>(sysCallWrapper);
    auto mountMonitor = std::make_shared<mount_monitor::mount_monitor::MountMonitor>(config, sysCallWrapper, fanotifyHandler->getFd());
    auto mountMonitorThread = std::make_unique<common::ThreadRunner>(mountMonitor, "mountMonitor", true);
    auto eventReader = std::make_shared<EventReaderThread>(fanotifyHandler->getFd(), sysCallWrapper, common::getPluginInstallPath(), scanRequestQueue);
    auto eventReaderThread = std::make_unique<common::ThreadRunner>(eventReader, "eventReader", false);

    std::string scanRequestSocketPath = common::getPluginInstallPath() / "chroot/var/scanning_socket";

    std::vector<std::shared_ptr<common::ThreadRunner>> scanHandlerThreads;
    for (int count = 0; count < MAX_SCAN_THREADS; ++count)
    {
        auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
            scanRequestQueue, scanRequestSocketPath);
        std::stringstream threadName;
        threadName << "scanHandler" << count;
        auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, threadName.str(), true);
        scanHandlerThreads.push_back(scanHandlerThread);
    }

    const int num_fds = 3;
    struct pollfd fds[num_fds];

    fds[0].fd = sigIntMonitor->monitorFd();
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = sigTermMonitor->monitorFd();
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    fds[2].fd = pipe.readFd();
    fds[2].events = POLLIN;
    fds[2].revents = 0;

    bool currentOaEnabledState = false;

    while (true)
    {
        // wait for an activity on one of the fds
        int activity = ::ppoll(fds, num_fds, nullptr, nullptr);
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
            pipe.notified();

            auto jsonString = OnAccessConfig::readConfigFile();
            OnAccessConfig::OnAccessConfiguration oaConfig = OnAccessConfig::parseOnAccessSettingsFromJson(jsonString);
            mountMonitor->setExcludeRemoteFiles(oaConfig.excludeRemoteFiles);

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
}
