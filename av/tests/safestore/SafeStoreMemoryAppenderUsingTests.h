// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "tests/common/TestSpecificDirectory.h"

namespace
{
    class SafeStoreMemoryAppenderUsingTests : public TestSpecificDirectory
    {
    public:
        SafeStoreMemoryAppenderUsingTests()
            : TestSpecificDirectory("safestore")
        {}
    };
}