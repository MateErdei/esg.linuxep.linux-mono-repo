//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/ISystemCallWrapper.h"
#include "mount_monitor/mountinfo/IMountInfo.h"
#include "mount_monitor/mount_monitor/OnAccessMountConfig.h"
#include "mount_monitor/mountinfo/ISystemPathsFactory.h"
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
            mountinfo::ISystemPathsFactorySharedPtr sysPathsFactory,
            struct timespec pollTimeout = {2,0});

        void updateConfig(std::vector<common::Exclusion> exclusions, bool excludeRemoteFiles);

        mountinfo::IMountPointSharedVector getAllMountpoints();
        mountinfo::IMountPointSharedVector getIncludedMountpoints(const mountinfo::IMountPointSharedVector& allMountPoints);
    private:
        void run() override;
        void markMounts(const mountinfo::IMountPointSharedVector& mounts);
        bool isIncludedFilesystemType(const mountinfo::IMountPointSharedPtr& mount);
        bool isIncludedMountpoint(const mountinfo::IMountPointSharedPtr& mount);
        bool setExcludeRemoteFiles(bool excludeRemoteFiles);
        bool setExclusions(std::vector<common::Exclusion> exclusions);

        OnAccessMountConfig& m_config;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        mountinfo::ISystemPathsFactorySharedPtr m_sysPathsFactory;
        const struct timespec m_pollTimeout;
        std::vector<common::Exclusion> m_exclusions;
    };
}
