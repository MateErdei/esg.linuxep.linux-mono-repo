// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessMountConfig.h"

#include "mount_monitor/mountinfo/IMountInfo.h"

#include "common/SigIntMonitor.h"
#include "common/SigTermMonitor.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        static int runSoapd();

        static mount_monitor::mountinfo::IMountPointSharedVector getIncludedMountpoints(
            const OnAccessMountConfig& config, const mount_monitor::mountinfo::IMountPointSharedVector& allMountpoints);
        static void innerRun(
            std::shared_ptr<common::SigIntMonitor>& sigIntMonitor,
            std::shared_ptr<common::SigTermMonitor>& sigTermMonitor,
            Common::Threads::NotifyPipe pipe);
    };
}
