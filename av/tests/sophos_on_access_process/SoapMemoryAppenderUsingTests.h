/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/MemoryAppender.h"

namespace
{
    class SoapMemoryAppenderUsingTests : public MemoryAppenderUsingTests
    {
    public:
        SoapMemoryAppenderUsingTests()
            : MemoryAppenderUsingTests("soapd_bootstrap")
        {}
    };
}
