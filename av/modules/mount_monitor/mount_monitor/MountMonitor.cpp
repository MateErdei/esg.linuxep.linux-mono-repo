/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MountMonitor.h"

#include "Logger.h"

#include "datatypes/AutoFd.h"
#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"

#include <cstring>
#include <sstream>

#include <poll.h>

namespace mount_monitor::mount_monitor
{
    MountMonitor::MountMonitor(
        OnAccessConfig& config,
        datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
        struct timespec pollTimeout)
    : m_config(config)
    , m_sysCalls(std::move(systemCallWrapper))
    , m_pollTimeout(pollTimeout)
    {

    }

    mountinfo::IMountPointSharedVector MountMonitor::getAllMountpoints()
    {
        auto pathsFactory = std::make_shared<mountinfoimpl::SystemPathsFactory>();
        auto mountInfo = std::make_shared<mountinfoimpl::Mounts>(pathsFactory->createSystemPaths());
        auto allMountpoints = mountInfo->mountPoints();
        LOGINFO("Found " << allMountpoints.size() << " mount points on the system");
        return allMountpoints;
    }

    mountinfo::IMountPointSharedVector MountMonitor::getIncludedMountpoints(mountinfo::IMountPointSharedVector allMountPoints)
    {
        mountinfo::IMountPointSharedVector includedMountpoints;
        for (const auto& mp : allMountPoints)
        {
            if ((mp->isHardDisc() && m_config.m_scanHardDisc) || (mp->isNetwork() && m_config.m_scanNetwork) ||
                (mp->isOptical() && m_config.m_scanOptical) || (mp->isRemovable() && m_config.m_scanRemovable))
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

    void MountMonitor::run()
    {
        // work out which filesystems are included based of config and mount information
        auto includedMountpoints = getIncludedMountpoints(getAllMountpoints());
        LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
        for (const auto& mp : includedMountpoints)
        {
            LOGDEBUG("Including mount point: " << mp->mountPoint());
        }

        LOGDEBUG("Setting poll timeout to " << m_pollTimeout.tv_sec << " seconds");

        datatypes::AutoFd mountsFd(open("/proc/mounts", O_RDONLY));
        if (!mountsFd.valid())
        {
            throw std::runtime_error("Failed to open /proc/mounts");
        }

        int exitFD = m_notifyPipe.readFd();
        const int num_fds = 2;
        struct pollfd fds[num_fds];

        fds[0].fd = exitFD;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        fds[1].fd = mountsFd.get();
        fds[1].events = POLLPRI;
        fds[1].revents = 0;

        announceThreadStarted();

        while (!stopRequested())
        {
            int activity = m_sysCalls->ppoll(fds, num_fds, &m_pollTimeout, nullptr);

            // Check if we should terminate before doing anything else
            if (stopRequested())
            {
                break;
            }

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

            if ((fds[0].revents & POLLIN) != 0)
            {
                LOGDEBUG("Stopping monitoring of mounts");
                break;
            }

            if ((fds[1].revents & POLLPRI) != 0)
            {
                LOGINFO("Mount points changed - re-evaluating");
                includedMountpoints = getIncludedMountpoints(getAllMountpoints());
                LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
                for (const auto& mp : includedMountpoints)
                {
                    LOGDEBUG("Including mount point: " << mp->mountPoint());
                }
            }
        }
    }
}
