/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "gmock/gmock.h"

#include <UpdateScheduler/IMapHostCacheId.h>

#include <string>

using namespace ::testing;

class MockMapHostCacheId : public UpdateScheduler::IMapHostCacheId
{
public:
    MOCK_CONST_METHOD1(cacheID, std::string(const std::string&));
};
