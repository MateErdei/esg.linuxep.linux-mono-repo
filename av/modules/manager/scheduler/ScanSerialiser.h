/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"

namespace manager::scheduler
{
    class ScanSerialiser
    {
    public:
        static std::string serialiseScan(const ScheduledScanConfiguration& config, const ScheduledScan& nextScan);
    };
}



