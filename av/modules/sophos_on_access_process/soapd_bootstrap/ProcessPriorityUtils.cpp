// Copyright 2022 Sophos Limited. All rights reserved.

#include "ProcessPriorityUtils.h"

#include "common/PidLockFile.h"
#include "common/PluginUtils.h"

#include <tuple>

void sophos_on_access_process::soapd_bootstrap::setThreatDetectorPriority(int level, const datatypes::ISystemCallWrapperSharedPtr& sysCalls)
{
    // Read PID file
    auto threatDetectorPidFile = common::getPluginInstallPath() / "chroot/var/threat_detector.pid";
    int pid = common::PidLockFile::getPidIfLocked(threatDetectorPidFile, sysCalls);

    if (pid == 0)
    {
        // ThreatDetector not running
        return;
    }

    // Set process priority
    sysCalls->setpriority(PRIO_PROCESS, pid, level);
}
