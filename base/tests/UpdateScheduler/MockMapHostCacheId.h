/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <string>
#include "gmock/gmock.h"
#include <UpdateScheduler/IMapHostCacheId.h>

using namespace ::testing;


class MockMapHostCacheId
        : public UpdateScheduler::IMapHostCacheId
{
public:

    MOCK_CONST_METHOD1(cacheID, std::string(const std::string&));

};

