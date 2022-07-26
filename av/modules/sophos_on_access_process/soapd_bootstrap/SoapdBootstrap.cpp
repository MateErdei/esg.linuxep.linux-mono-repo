// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SoapdBootstrap.h"

#include "common/SigIntMonitor.h"
#include "common/SigTermMonitor.h"

#include <memory>

#include <poll.h>
#include <unistd.h>

int sophos_on_access_process::soapd_bootstrap::SoapdBootstrap::runSoapd()
{
    // Setup logging

    // Implement soapd

    std::shared_ptr<common::SigIntMonitor> sigIntMonitor{common::SigIntMonitor::getSigIntMonitor()};
    std::shared_ptr<common::SigTermMonitor> sigTermMonitor{common::SigTermMonitor::getSigTermMonitor()};


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
