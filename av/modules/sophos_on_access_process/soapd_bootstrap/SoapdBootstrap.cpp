// Copyright 2022, Sophos Limited.  All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
// Component
#include "sophos_on_access_process/OnAccessConfig//OnAccessConfigMonitor.h"
#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"
// Product
#include "common/FDUtils.h"
#include "common/PidLockFile.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
// Base
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Std C++
#include <fstream>
#include <memory>
// Std C
#include <poll.h>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;


int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");

    // Take soapd lock file
    fs::path lockfile = pluginInstall / "var/soapd.pid";
    common::PidLockFile lock(lockfile);

    std::shared_ptr<common::SigIntMonitor> sigIntMonitor{common::SigIntMonitor::getSigIntMonitor()};
    std::shared_ptr<common::SigTermMonitor> sigTermMonitor{common::SigTermMonitor::getSigTermMonitor()};

    fs::path socketPath = pluginInstall / "var/soapd_controller";
    LOGINFO("Socket is at: " << socketPath);

    Common::Threads::NotifyPipe onAccessConfigPipe;
    OnAccessConfig::OnAccessConfigMonitor configMonitor(socketPath, onAccessConfigPipe);
    configMonitor.start();

    innerRun(sigIntMonitor, sigTermMonitor, onAccessConfigPipe);

    LOGINFO("Stopping On Access config monitor");
    configMonitor.requestStop();
    LOGINFO("Joining On Access config monitor");
    configMonitor.join();

    LOGINFO("Exiting soapd");

    return 0;
}

void SoapdBootstrap::innerRun(
    std::shared_ptr<common::SigIntMonitor>& sigIntMonitor,
    std::shared_ptr<common::SigTermMonitor>& sigTermMonitor,
    Common::Threads::NotifyPipe pipe)
{
    mount_monitor::mount_monitor::OnAccessMountConfig config;
    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
    auto mountMonitor = std::make_unique<mount_monitor::mount_monitor::MountMonitor>(config, sysCallWrapper);
    auto mountMonitorThread = std::make_unique<common::ThreadRunner>(*mountMonitor, "scanProcessMonitor");

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
        }
    }
    scanRequestQueue->stop();
}
