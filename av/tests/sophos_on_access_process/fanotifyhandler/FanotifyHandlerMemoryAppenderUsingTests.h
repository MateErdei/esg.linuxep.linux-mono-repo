//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/MemoryAppender.h"

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
