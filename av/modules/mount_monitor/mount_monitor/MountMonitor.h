//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/ISystemCallWrapper.h"
#include "mount_monitor/mountinfo/IMountInfo.h"
#include "mount_monitor/mount_monitor/OnAccessMountConfig.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/Exclusion.h"

#include <map>

namespace fanotifyhandler = sophos_on_access_process::fanotifyhandler;

namespace mount_monitor::mount_monitor
{
    class MountMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        MountMonitor(
            OnAccessMountConfig& config,
            datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
            fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
            struct timespec pollTimeout = {2,0});

        void setExcludeRemoteFiles(bool excludeRemoteFiles);
        void setExclusions(std::vector<common::Exclusion> exclusions);

        mountinfo::IMountPointSharedVector getAllMountpoints();
        mountinfo::IMountPointSharedVector getIncludedMountpoints(mountinfo::IMountPointSharedVector allMountPoints);
    private:
        void run() override;
        void markMounts(const mountinfo::IMountPointSharedVector& mounts);
        bool isIncludedFilesystemType(mountinfo::IMountPointSharedPtr mount);
        bool isIncludedMountpoint(mountinfo::IMountPointSharedPtr mount);

        OnAccessMountConfig& m_config;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        const struct timespec m_pollTimeout;
        std::vector<common::Exclusion> m_exclusions;
    };
}
