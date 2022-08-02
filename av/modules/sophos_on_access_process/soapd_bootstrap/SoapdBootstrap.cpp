// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SoapdBootstrap.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "common/ThreadRunner.h"
#include "datatypes/SystemCallWrapper.h"
#include "sophos_on_access_process/OnAccessConfig//OnAccessConfigMonitor.h"
#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "common/FDUtils.h"

#include <memory>
#include <fstream>

#include <poll.h>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;

void write_pid_file(const fs::path& pluginInstall)
{
    fs::path pidFilePath = pluginInstall / "var/soapd.pid";
    fs::remove(pidFilePath);
    pid_t pid = getpid();
    LOGDEBUG("Writing Pid: " << pid << " to: " << pidFilePath);
    std::ofstream ofs(pidFilePath, std::ofstream::trunc);
    ofs << pid;
    ofs.close();
    fs::permissions(pidFilePath,
                    fs::perms::group_read | fs::perms::others_read,
                    fs::perm_options::add);
}

int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    write_pid_file(pluginInstall);
    // Implement soapd

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

            LOGERROR("Failed to read from pipe - shutting down. Error: " << strerror(error) << " (" << error << ')');
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
}
