// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SoapdBootstrap.h"

#include "Logger.h"

#include "common/SigIntMonitor.h"
#include "common/SigTermMonitor.h"
#include "datatypes/AutoFd.h"

#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"

#include <cstring>
#include <memory>

#include <poll.h>

using namespace sophos_on_access_process::soapd_bootstrap;

mount_monitor::mountinfo::IMountPointSharedVector SoapdBootstrap::getAllMountpoints()
{
    auto pathsFactory = std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>();
    auto mountInfo = std::make_shared<mount_monitor::mountinfoimpl::Mounts>(pathsFactory->createSystemPaths());
    auto allMountpoints = mountInfo->mountPoints();
    LOGINFO("Found " << allMountpoints.size() << " mount points on the system");
    return allMountpoints;
}

mount_monitor::mountinfo::IMountPointSharedVector SoapdBootstrap::getIncludedMountpoints(
    const OnAccessConfig& config, const mount_monitor::mountinfo::IMountPointSharedVector& allMountpoints)
{
    mount_monitor::mountinfo::IMountPointSharedVector includedMountpoints;
    for (const auto& mp : allMountpoints)
    {
        if ((mp->isHardDisc() && config.m_scanHardDisc) || (mp->isNetwork() && config.m_scanNetwork) ||
            (mp->isOptical() && config.m_scanOptical) || (mp->isRemovable() && config.m_scanRemovable))
        {
            includedMountpoints.push_back(mp);
        }
        else if (mp->isSpecial())
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is system and will be excluded from the scan");
        }
        else
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " has been excluded from the scan");
        }
    }
    return includedMountpoints;
}

int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    // Implement soapd

    std::shared_ptr<common::SigIntMonitor> sigIntMonitor{common::SigIntMonitor::getSigIntMonitor()};
    std::shared_ptr<common::SigTermMonitor> sigTermMonitor{common::SigTermMonitor::getSigTermMonitor()};

    OnAccessConfig config;
    // work out which filesystems are included based of config and mount information
    auto includedMountpoints = getIncludedMountpoints(config, getAllMountpoints());
    LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
    for (const auto& mp : includedMountpoints)
    {
        LOGDEBUG("Including mount point: " << mp->mountPoint());
    }

    datatypes::AutoFd mountsFd(open("/proc/mounts", O_RDONLY));
    if (!mountsFd.valid())
    {
        throw std::runtime_error("Failed to open /proc/mounts");
    }

    const int num_fds = 3;
    struct pollfd fds[num_fds];

    fds[0].fd = sigIntMonitor->monitorFd();
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = sigTermMonitor->monitorFd();
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    fds[2].fd = mountsFd.get();
    fds[2].events = POLLPRI;
    fds[2].revents = 0;

    while (true)
    {
        int activity = ::ppoll(fds, num_fds, nullptr, nullptr);

        if (activity < 0)
        {
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from ppoll");
                continue;
            }

            LOGERROR("Failed to monitor config. Error: " << strerror(error)<< " (" << error << ')');
            break;
        }
        else if (activity == 0)
        {
            // Timed out - will attempt to reconnect if not connected
            continue;
        }

        if ((fds[2].revents & POLLPRI) != 0)
        {
            LOGINFO("Mount points changed - re-evaluating");
            includedMountpoints = getIncludedMountpoints(config, getAllMountpoints());
            LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
            for (const auto& mp : includedMountpoints)
            {
                LOGDEBUG("Including mount point: " << mp->mountPoint());
            }
        }
    }

    return 0;
}
