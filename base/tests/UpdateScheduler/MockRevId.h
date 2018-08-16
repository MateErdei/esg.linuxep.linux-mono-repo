/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "gmock/gmock.h"
#include "UpdateScheduler/IRevId.h"

using namespace ::testing;


class MockRevId: public UpdateScheduler::IRevId
{
public:

    MOCK_CONST_METHOD0(revID, std::string(void));

};


