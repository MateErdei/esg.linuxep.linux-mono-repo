/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"

#include <Common/Threads/AbstractThread.h>

namespace manager::scheduler
{
    class ScanScheduler : public Common::Threads::AbstractThread
    {
    public:
        void run() override;
        void updateConfig(ScheduledScanConfiguration);

    };
}



