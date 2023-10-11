// Copyright 2022 Sophos Limited. All rights reserved.

#include "MountMonitor.h"

#include "Logger.h"

#include "common/SaferStrerror.h"
#include "datatypes/AutoFd.h"
#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "sophos_on_access_process/local_settings/OnAccessProductConfigDefaults.h"
#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryFields.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <poll.h>

#include <sstream>

using namespace sophos_on_access_process::OnAccessConfig;

namespace mount_monitor::mount_monitor
{
    MountMonitor::MountMonitor(
        OnAccessConfiguration& config,
        datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
        fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
        mountinfo::ISystemPathsFactorySharedPtr sysPathsFactory,
        struct timespec pollTimeout)
    : m_config(config)
    , m_sysCalls(std::move(systemCallWrapper))
    , m_fanotifyHandler(std::move(fanotifyHandler))
    , m_sysPathsFactory(sysPathsFactory)
    , m_pollTimeout(pollTimeout)
    {
    }

    mountinfo::IMountPointSharedVector MountMonitor::getAllMountpoints()
    {
        try
        {
            auto mountInfo = std::make_shared<mountinfoimpl::Mounts>(m_sysPathsFactory->createSystemPaths());
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
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is a Sophos SPL bind mount and will be excluded from scanning");
            return false;
        }
        else
        {
            auto fsMountPoint = fs::path(mp->mountPoint());
            bool isDir = mp->isDirectory();
            for (const auto& exclusion: m_config.exclusions)
            {
                if (exclusion.appliesToPath(fsMountPoint, isDir, !isDir))
                {
                    LOGDEBUG("Mount point " << fsMountPoint.c_str() << " matches an exclusion in the policy and will be excluded from scanning");
                    return false;
                }
            }
        }
        return true;
    }

    bool MountMonitor::isIncludedFilesystemType(const mountinfo::IMountPointSharedPtr& mp)
    {
        auto fileSystemType = mp->filesystemType();
        if (FILE_SYSTEMS_TO_EXCLUDE.find(fileSystemType) != FILE_SYSTEMS_TO_EXCLUDE.cend())
        {
            LOGINFO("Mount point " << mp->mountPoint().c_str() << " using filesystem "
                                   << fileSystemType << " is not supported and will be excluded from scanning");
            return false;
        }
        else if (mp->isSpecial())
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is system and will be excluded from scanning");
            return false;
        }
        else if (mp->isHardDisc() || (mp->isNetwork() && !m_config.excludeRemoteFiles) || mp->isRemovable() || mp->isOptical())
        {
            return true;
        }
        else
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " has been excluded from scanning");
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
            if (m_config.enabled)
            {
                LOGINFO("OA config changed, re-enumerating mount points");
                markMounts(getAllMountpoints());
            }
        }
    }

    void MountMonitor::markMounts(const mountinfo::IMountPointSharedVector& allMounts)
    {
        std::set<std::string> fileSystemSet;
        int count = 0;
        for (const auto& mount: allMounts)
        {
            if (stopRequested())
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
                fileSystemSet.emplace(mount->filesystemType());
            }
            else
            {
                std::ignore = m_fanotifyHandler->unmarkMount(mountPointStr);
                LOGTRACE("Excluding mount point: " << mountPointStr);
            }
        }
        addFileSystemToTelemetry(fileSystemSet);
        LOGDEBUG("Including " << count << " mount points in on-access scanning");
    }

    void MountMonitor::addFileSystemToTelemetry(std::set<std::string>& fileSystemList)
    {
        //We do all this instead of addValueToSetInternal because we want it to stick
        if (fileSystemList.size() == 0)
        {
            //We would have fail to mark errors in the log
            return;
        }

        if (fileSystemList.size() > TELEMETRY_FILE_SYSTEM_LIST_MAX)
        {
            auto begin = fileSystemList.begin();
            std::advance(begin, TELEMETRY_FILE_SYSTEM_LIST_MAX),
            fileSystemList.erase(begin, fileSystemList.end());
        }

        using namespace Common::Telemetry;

        std::list<TelemetryObject> fsTelemetryList;

        for (const auto& fileSystem : fileSystemList)
        {
            TelemetryValue fsVal(fileSystem);
            TelemetryObject fsObj;
            fsObj.set(fsVal);
            fsTelemetryList.push_back(fsObj);
        }

        TelemetryObject endObject;
        endObject.set(fsTelemetryList);
        TelemetryHelper::getInstance().set(sophos_on_access_process::onaccessimpl::onaccesstelemetry::FILE_SYSTEM_TYPES_STR, endObject, true);
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
