// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "unixsocket/processControllerSocket/IProcessControlMessageCallback.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockThreatDetectorControlCallbacks : public unixsocket::IProcessControlMessageCallback
    {
    public:
        MockThreatDetectorControlCallbacks() {}

        MOCK_METHOD(void, processControlMessage, (const scan_messages::E_COMMAND_TYPE& command), (override));
    };
}
