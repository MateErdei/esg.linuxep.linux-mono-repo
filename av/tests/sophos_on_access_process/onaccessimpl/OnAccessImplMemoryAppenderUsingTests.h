// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "tests/common/MemoryAppender.h"

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
