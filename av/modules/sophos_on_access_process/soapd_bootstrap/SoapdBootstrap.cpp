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
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
// Base
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Std C++
#include <memory>
// Std C
#include <poll.h>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;

int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    // Implement soapd

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path socketPath = pluginInstall / "var/soapd_controller";
    LOGINFO("Socket is at: " << socketPath);

    Common::Threads::NotifyPipe onAccessConfigPipe;
    Common::Threads::NotifyPipe usePolicyOverridePipe;
    Common::Threads::NotifyPipe useFlagOverridePipe;
    OnAccessConfig::OnAccessConfigMonitor configMonitor(socketPath,
                                                        onAccessConfigPipe,
                                                        usePolicyOverridePipe,
                                                        useFlagOverridePipe);
    configMonitor.start();

    innerRun(sigIntMonitor, sigTermMonitor, onAccessConfigPipe, usePolicyOverridePipe, useFlagOverridePipe);

    LOGINFO("Stopping On Access config monitor");
    configMonitor.requestStop();
    LOGINFO("Joining On Access config monitor");
    configMonitor.join();

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
    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
    auto mountMonitor = std::make_unique<mount_monitor::mount_monitor::MountMonitor>(config, sysCallWrapper);
    auto mountMonitorThread = std::make_unique<common::ThreadRunner>(*mountMonitor, "scanProcessMonitor");

    struct pollfd fds[] {
        { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = onAccessConfigPipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = usePolicyOverridePipe.readFd(), .events = POLLIN, .revents = 0 },
        { .fd = useFlagOverridePipe.readFd(), .events = POLLIN, .revents = 0 },
    };

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

            auto jsonString = OnAccessConfig::readConfigFile();
            OnAccessConfig::OnAccessConfiguration oaConfig = OnAccessConfig::parseOnAccessSettingsFromJson(jsonString);
            mountMonitor->setExcludeRemoteFiles(oaConfig.excludeRemoteFiles);
        }

        if ((fds[3].revents & POLLIN) != 0)
        {
            usePolicyOverridePipe.notified();

            //use policy settings here
            LOGINFO("Using on-access settings from policy...");
        }

        if ((fds[4].revents & POLLIN) != 0)
        {
            useFlagOverridePipe.notified();

            //stop OA here
            LOGINFO("Disabling on-access...");
        }
    }
}
