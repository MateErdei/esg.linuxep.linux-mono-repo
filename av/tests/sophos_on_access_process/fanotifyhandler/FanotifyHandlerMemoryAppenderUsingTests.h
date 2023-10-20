// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "tests/common/MemoryAppender.h"

namespace
{
    class FanotifyHandlerMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        FanotifyHandlerMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("fanotifyhandler")
        {}
    };
}
