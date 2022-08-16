/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/ISystemCallWrapper.h"
#include "mount_monitor/mount_monitor/OnAccessConfig.h"
#include "mount_monitor/mountinfo/IMountInfo.h"

#include "Common/Threads/AbstractThread.h"

#include <map>

namespace mount_monitor::mount_monitor
{
    class MountMonitor : public Common::Threads::AbstractThread
    {
    public:
        MountMonitor(
            OnAccessConfig& config,
            datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
            int faNotifyFd,
            struct timespec pollTimeout = {2,0});

        void setExcludeRemoteFiles(bool excludeRemoteFiles);

        mountinfo::IMountPointSharedVector getAllMountpoints();
        mountinfo::IMountPointSharedVector getIncludedMountpoints(mountinfo::IMountPointSharedVector allMountPoints);
    private:
        void run() override;
        void markMounts(mountinfo::IMountPointSharedVector mounts);

        OnAccessConfig& m_config;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        int m_faNotifyFd;
        const struct timespec m_pollTimeout;
    };
}
