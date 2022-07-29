// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SoapdBootstrap.h"

#include "Logger.h"

#include "common/SigIntMonitor.h"
#include "common/SigTermMonitor.h"
#include "common/ThreadRunner.h"
#include "datatypes/SystemCallWrapper.h"

#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mount_monitor/OnAccessConfig.h"

#include <memory>

#include <poll.h>

using namespace sophos_on_access_process::soapd_bootstrap;

int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    // Implement soapd

    std::shared_ptr<common::SigIntMonitor> sigIntMonitor{common::SigIntMonitor::getSigIntMonitor()};
    std::shared_ptr<common::SigTermMonitor> sigTermMonitor{common::SigTermMonitor::getSigTermMonitor()};

    mount_monitor::mount_monitor::OnAccessConfig config;
    auto sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
    auto mountMonitor = std::make_unique<mount_monitor::mount_monitor::MountMonitor>(config, sysCallWrapper);
    auto mountMonitorThread = std::make_unique<common::ThreadRunner>(*mountMonitor, "scanProcessMonitor");

    const int num_fds = 2;
    struct pollfd fds[num_fds];

    fds[0].fd = sigIntMonitor->monitorFd();
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = sigTermMonitor->monitorFd();
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    ::ppoll(fds, num_fds, nullptr, nullptr);

    return 0;
}
