//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/MemoryAppender.h"

namespace
{
    class FANotifyHandlerMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        FANotifyHandlerMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("fanotifyhandler")
        {}
    };
}
