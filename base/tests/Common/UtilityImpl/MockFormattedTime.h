/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "gmock/gmock.h"

#include "modules/Common/UtilityImpl/TimeUtils.h"

using namespace ::testing;

class MockFormattedTime : public Common::UtilityImpl::IFormattedTime
{
public:
    MOCK_CONST_METHOD0(currentTime, std::string(void));
    MOCK_CONST_METHOD0(currentEpochTimeInSeconds, std::string(void));
    MOCK_CONST_METHOD0(currentEpochTimeInSecondsAsInteger, u_int64_t(void));
    MOCK_CONST_METHOD0(bootTime, std::string(void));
};
