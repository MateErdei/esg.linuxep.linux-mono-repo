/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "tests/common/TestSpecificDirectory.h"

namespace
{
    class UnixSocketMemoryAppenderUsingTests : public TestSpecificDirectory
    {
    public:
        UnixSocketMemoryAppenderUsingTests()
            : TestSpecificDirectory("UnixSocket")
        {}
    };
}
