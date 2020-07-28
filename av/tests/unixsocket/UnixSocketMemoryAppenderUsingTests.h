/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "tests/common/MemoryAppender.h"

namespace
{
    class UnixSocketMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        UnixSocketMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("UnixSocket")
        {}
    };
}
