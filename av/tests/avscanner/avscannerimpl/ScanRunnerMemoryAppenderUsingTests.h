/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "tests/common/MemoryAppender.h"

namespace
{
    class ScanRunnerMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        ScanRunnerMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("NamedScanRunner")
        {}
    };
}
