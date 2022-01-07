/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "datatypes/ISysCalls.h"
#include <gmock/gmock.h>

namespace
{
    class MockSysCallsWrapper : public datatypes::ISysCalls
    {
    public:
        MOCK_METHOD0(getSystemUpTime, std::pair<const int, const long>());
    };
}
