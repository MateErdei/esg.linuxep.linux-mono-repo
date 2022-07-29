/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/MemoryAppender.h"

namespace
{
    class MountMonitorMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        MountMonitorMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("MountMonitor")
        {}
    };
}
