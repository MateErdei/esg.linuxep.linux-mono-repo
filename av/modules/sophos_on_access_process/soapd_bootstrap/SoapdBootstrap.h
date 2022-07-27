// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "OnAccessConfig.h"

#include "mount_monitor/mountinfo/IMountInfo.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        static int runSoapd();

        static mount_monitor::mountinfo::IMountPointSharedVector getIncludedMountpoints(
            const OnAccessConfig& config, const mount_monitor::mountinfo::IMountPointSharedVector& allMountpoints);
    };
}
