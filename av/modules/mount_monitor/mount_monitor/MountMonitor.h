//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/ISystemCallWrapper.h"
#include "mount_monitor/mountinfo/IMountInfo.h"
#include "mount_monitor/mount_monitor/OnAccessMountConfig.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"

#include "common/AbstractThreadPluginInterface.h"

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

        mountinfo::IMountPointSharedVector getAllMountpoints();
        mountinfo::IMountPointSharedVector getIncludedMountpoints(mountinfo::IMountPointSharedVector allMountPoints);
    private:
        void run() override;
        void markMounts(mountinfo::IMountPointSharedVector mounts);

        OnAccessMountConfig& m_config;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        const struct timespec m_pollTimeout;
    };
}
