//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "datatypes/ISystemCallWrapper.h"
#include "mount_monitor/mountinfo/IMountInfo.h"
#include "sophos_on_access_process/soapd_bootstrap/OnAccessConfigurationUtils.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/Exclusion.h"

#include <map>

namespace fanotifyhandler = sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::OnAccessConfig;

namespace mount_monitor::mount_monitor
{
    class MountMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        MountMonitor(
            OnAccessConfiguration& config,
            datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
            fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
            struct timespec pollTimeout = {2,0});

        void updateConfig(const OnAccessConfiguration& config);

        mountinfo::IMountPointSharedVector getAllMountpoints();
        mountinfo::IMountPointSharedVector getIncludedMountpoints(const mountinfo::IMountPointSharedVector& allMountPoints);
    private:
        void run() override;
        void markMounts(const mountinfo::IMountPointSharedVector& mounts);
        bool isIncludedFilesystemType(const mountinfo::IMountPointSharedPtr& mount);
        bool isIncludedMountpoint(const mountinfo::IMountPointSharedPtr& mount);

    TEST_PUBLIC:
        void addFileSystemToTelemetry(const std::map<std::string, bool>& fileSystemList);

    private:
        OnAccessConfiguration& m_config;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        const struct timespec m_pollTimeout;
    };
}
