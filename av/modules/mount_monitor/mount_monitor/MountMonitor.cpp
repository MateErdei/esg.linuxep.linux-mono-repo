// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MountMonitor.h"

#include "Logger.h"

#include "common/SaferStrerror.h"
#include "datatypes/AutoFd.h"
#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <sstream>

#include <poll.h>

using namespace sophos_on_access_process::OnAccessConfig;

namespace mount_monitor::mount_monitor
{
    MountMonitor::MountMonitor(
        OnAccessConfiguration& config,
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

    bool MountMonitor::isIncludedMountpoint(const mountinfo::IMountPointSharedPtr& mp)
    {
        if (mp->mountPoint().rfind("/opt/sophos-spl/", 0) == 0)
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is a Sophos SPL bind mount and will be excluded from the scan");
            return false;
        }
        else
        {
            for (const auto& exclusion: m_config.exclusions)
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

    bool MountMonitor::isIncludedFilesystemType(const mountinfo::IMountPointSharedPtr& mp)
    {
        if (mp->isHardDisc() || (mp->isNetwork() && !m_config.excludeRemoteFiles) || mp->isRemovable() || mp->isOptical())
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

    mountinfo::IMountPointSharedVector MountMonitor::getIncludedMountpoints(const mountinfo::IMountPointSharedVector& allMountPoints)
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

    void MountMonitor::updateConfig(const OnAccessConfiguration& config)
    {
        if (config != m_config)
        {
            m_config = config;
            LOGINFO("OA config changed, re-enumerating mount points");
            markMounts(getAllMountpoints());
        }
    }

    void MountMonitor::markMounts(const mountinfo::IMountPointSharedVector& allMounts)
    {
        //The bool is superfluous, we want unique list of keys
        std::map<std::string, bool> fileSystemMap;
        int count = 0;
        for (const auto& mount: allMounts)
        {
            if (stopRequested() || !m_fanotifyHandler->isInitialised())
            {
                break;
            }

            std::string mountPointStr = mount->mountPoint();
            if (isIncludedFilesystemType(mount) && isIncludedMountpoint(mount))
            {
                int ret = m_fanotifyHandler->markMount(mountPointStr);
                if (ret == -1)
                {
                    LOGWARN(
                        "Unable to mark fanotify for mount point " << mountPointStr << ": "
                                                                   << common::safer_strerror(errno)
                                                                   << ". On Access Scanning disabled on the mount");
                    continue;
                }
                count++;
                LOGDEBUG("Including mount point: " << mountPointStr);
                fileSystemMap.try_emplace(mount->filesystemType(), true);
            }
            else
            {
                m_fanotifyHandler->unmarkMount(mountPointStr);
                LOGTRACE("Excluding mount point: " << mountPointStr);
            }
        }
        addFileSystemToTelemetry(fileSystemMap);
        LOGDEBUG("Including " << count << " mount points in on-access scanning");
    }

    void MountMonitor::addFileSystemToTelemetry(std::map<std::string, bool>& fileSystemList)
    {
        //We do all this instead of addValueToSetInternal because we want it to stick
        assert(fileSystemList.size() > 0);

        if (fileSystemList.size() > telemetryFileSystemListMax)
        {
            auto begin = fileSystemList.begin();
            std::advance(begin, telemetryFileSystemListMax),
            fileSystemList.erase(begin, fileSystemList.end());
        }

        using namespace Common::Telemetry;

        std::list<TelemetryObject> fsTelemetryList;

        for (auto fileSystem : fileSystemList)
        {
            TelemetryValue fsVal(fileSystem.first);
            TelemetryObject fsObj;
            fsObj.set(fsVal);
            fsTelemetryList.push_back(fsObj);
        }

        TelemetryObject endObject;
        endObject.set(fsTelemetryList);
        TelemetryHelper::getInstance().set("file-system-types", endObject, true);
    }

    void MountMonitor::run()
    {
        // work out which filesystems are included based of config and mount information
        markMounts(getAllMountpoints());

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
                markMounts(getAllMountpoints());
            }
        }
    }
}
