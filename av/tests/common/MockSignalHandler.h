// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <common/signals/SigTermMonitor.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockSignalHandler : public common::signals::ISignalHandlerBase
    {
    public:
        MockSignalHandler()
        {
            ON_CALL(*this, triggered).WillByDefault(Return(false));
            ON_CALL(*this, monitorFd).WillByDefault(Return(0));
        }

        MOCK_METHOD(bool, triggered, ());
        MOCK_METHOD(int, monitorFd, ());
    };
}