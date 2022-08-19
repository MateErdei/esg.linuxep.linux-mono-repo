//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/MemoryAppender.h"

namespace
{
    class OnAccessImplMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        OnAccessImplMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("OnAccessImpl")
        {}
    };
}
