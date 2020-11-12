/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "tests/common/MemoryAppender.h"

namespace
{
    class FileWalkerMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        FileWalkerMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("FileWalker")
        {}
    };
}
