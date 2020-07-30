/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "tests/common/MemoryAppender.h"

namespace
{
    class ScanProcessMonitorMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        ScanProcessMonitorMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("ScanProcessMonitor")
        {}
    };
}
