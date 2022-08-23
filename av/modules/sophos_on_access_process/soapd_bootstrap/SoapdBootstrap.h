// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "mount_monitor/mount_monitor/MountMonitor.h"

#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    class SoapdBootstrap
    {
    public:
        static int runSoapd();

        static void innerRun(
            std::shared_ptr<common::signals::SigIntMonitor>& sigIntMonitor,
            std::shared_ptr<common::signals::SigTermMonitor>& sigTermMonitor,
            Common::Threads::NotifyPipe onAccessConfigPipe,
            Common::Threads::NotifyPipe usePolicyOverridePipe,
            Common::Threads::NotifyPipe useFlagOverridePipe);
    };
}
