/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/OSUtilities/ISystemUtils.h"

#include <gmock/gmock.h>
#include <string>
#include <functional>

using namespace ::testing;

class MockSystemUtils : public OSUtilities::ISystemUtils
{
public:
    MOCK_CONST_METHOD1(getEnvironmentVariable, std::string(const std::string&));
};
