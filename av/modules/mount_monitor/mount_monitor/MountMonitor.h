//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "datatypes/ISystemCallWrapper.h"
#include "mount_monitor/mountinfo/IMountInfo.h"
#include "mount_monitor/mountinfo/ISystemPathsFactory.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"
#include "sophos_on_access_process/soapd_bootstrap/OnAccessConfigurationUtils.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/Exclusion.h"

#include <set>

namespace fanotifyhandler = sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::OnAccessConfig;

namespace mount_monitor::mount_monitor
{
    class MountMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        static constexpr int TELEMETRY_FILE_SYSTEM_LIST_MAX = 100;

        MountMonitor(
            OnAccessConfiguration& config,
            datatypes::ISystemCallWrapperSharedPtr systemCallWrapper,
            fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
            mountinfo::ISystemPathsFactorySharedPtr sysPathsFactory);

        void updateConfig(const OnAccessConfiguration& config);

        mountinfo::IMountPointSharedVector getAllMountpoints();
        mountinfo::IMountPointSharedVector getIncludedMountpoints(const mountinfo::IMountPointSharedVector& allMountPoints);

    private:
        void run() override;
        bool isIncludedMountpoint(const mountinfo::IMountPointSharedPtr& mount);

    TEST_PUBLIC:
        void markMounts(const mountinfo::IMountPointSharedVector& mounts);
        void addFileSystemToTelemetry(std::set<std::string>& fileSystemList);
        bool isIncludedFilesystemType(const mountinfo::IMountPointSharedPtr& mount);

    private:
        OnAccessConfiguration& m_config;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        mountinfo::ISystemPathsFactorySharedPtr m_sysPathsFactory;

    };
}
