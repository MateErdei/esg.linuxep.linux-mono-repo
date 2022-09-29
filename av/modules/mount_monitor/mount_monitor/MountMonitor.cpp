// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MountMonitor.h"

#include "Logger.h"

#include "common/SaferStrerror.h"
#include "datatypes/AutoFd.h"
#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"

#include <sstream>

#include <poll.h>

namespace mount_monitor::mount_monitor
{
    MountMonitor::MountMonitor(
        OnAccessMountConfig& config,
        datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
        fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
        struct timespec pollTimeout)
    : m_config(config)
    , m_sysCalls(std::move(systemCallWrapper))
    , m_fanotifyHandler(std::move(fanotifyHandler))
    , m_pollTimeout(pollTimeout)
    {
    }

    mountinfo::IMountPointSharedVector MountMonitor::getAllMountpoints()
    {
        auto pathsFactory = std::make_shared<mountinfoimpl::SystemPathsFactory>();
        try
        {
            auto mountInfo = std::make_shared<mountinfoimpl::Mounts>(pathsFactory->createSystemPaths());
            auto allMountpoints = mountInfo->mountPoints();
            LOGINFO("Found " << allMountpoints.size() << " mount points on the system");
            return allMountpoints;
        }
        catch (const std::runtime_error& e)
        {
            LOGFATAL(e.what());
            throw;
        }
    }

    bool MountMonitor::isIncludedMountpoint(mountinfo::IMountPointSharedPtr mp)
    {
        if (mp->mountPoint().rfind("/opt/sophos-spl/", 0) == 0)
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is a Sophos SPL bind mount and will be excluded from the scan");
            return false;
        }
        else
        {
            for (const auto& exclusion: m_exclusions)
            {
                bool isDir = mp->isDirectory();
                if (exclusion.appliesToPath(mp->mountPoint(), isDir, !isDir))
                {
                    LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " matches an exclusion in the policy and will be excluded from the scan");
                    return false;
                }
            }
        }
        return true;
    }

    bool MountMonitor::isIncludedFilesystemType(mountinfo::IMountPointSharedPtr mp)
    {
        if ((mp->isHardDisc() && m_config.m_scanHardDisc) || (mp->isNetwork() && m_config.m_scanNetwork) ||
            (mp->isOptical() && m_config.m_scanOptical) || (mp->isRemovable() && m_config.m_scanRemovable))
        {
            return true;
        }
        else if (mp->isSpecial())
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is system and will be excluded from the scan");
            return false;
        }
        else
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " has been excluded from the scan");
            return false;
        }
    }

    mountinfo::IMountPointSharedVector MountMonitor::getIncludedMountpoints(mountinfo::IMountPointSharedVector allMountPoints)
    {
        mountinfo::IMountPointSharedVector includedMountpoints;
        for (const auto& mp : allMountPoints)
        {
            if (isIncludedFilesystemType(mp) && isIncludedMountpoint(mp))
            {
                includedMountpoints.push_back(mp);
            }
        }
        return includedMountpoints;
    }

    void MountMonitor::setExcludeRemoteFiles(bool excludeRemoteFiles)
    {
        bool scanNetwork = !excludeRemoteFiles;
        if (scanNetwork != m_config.m_scanNetwork)
        {
            LOGINFO("OA config changed, re-enumerating mount points");
            m_config.m_scanNetwork = scanNetwork;
            auto includedMountpoints = getIncludedMountpoints(getAllMountpoints());
            LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
            for (const auto& mp : includedMountpoints)
            {
                LOGDEBUG("Including mount point: " << mp->mountPoint());
            }
        }
    }

    void MountMonitor::setExclusions(std::vector<common::Exclusion> exclusions)
    {
        if (exclusions != m_exclusions)
        {
            LOGINFO("OA config changed, re-enumerating mount points");
            m_exclusions = exclusions;
            auto includedMountpoints = getIncludedMountpoints(getAllMountpoints());
            LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
            for (const auto& mp : includedMountpoints)
            {
                LOGDEBUG("Including mount point: " << mp->mountPoint());
            }
        }
    }

    void MountMonitor::markMounts(const mountinfo::IMountPointSharedVector& mounts)
    {
        LOGDEBUG("Including " << mounts.size() << " mount points in on-access scanning");
        for (const auto& mount: mounts)
        {
            std::string mountPointStr = mount->mountPoint();
            int ret = m_fanotifyHandler->markMount(mountPointStr);
            if (ret == -1)
            {
                LOGWARN("Unable to mark fanotify for mount point " << mountPointStr << ": " << common::safer_strerror(errno) << ". On Access Scanning disabled on the mount");
                return;
            }
            LOGDEBUG("Including mount point: " << mountPointStr);
        }
    }

    void MountMonitor::run()
    {
        // work out which filesystems are included based of config and mount information
        markMounts(getIncludedMountpoints(getAllMountpoints()));

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

                LOGERROR("Failed to monitor config. Error: "
                         << common::safer_strerror(error)<< " (" << error << ')');
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
                // Mark all current mounts
                // Will override previous marks for same mounts
                // Fanotify automatically unmarks mounts that are unmounted
                LOGINFO("Mount points changed - re-evaluating");
                markMounts(getIncludedMountpoints(getAllMountpoints()));
            }
        }
    }
}
