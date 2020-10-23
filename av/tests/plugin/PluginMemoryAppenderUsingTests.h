/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "tests/common/MemoryAppender.h"

namespace
{
    class PluginMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        PluginMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("av")
        {}
    };
}
