/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/Reactor/IShutdownListener.h"
#include <gmock/gmock.h>
using namespace ::testing;
using namespace Common::Reactor;

class MockShutdownListener : public IShutdownListener
{
public:
    MOCK_METHOD0(notifyShutdownRequested, void());
};
