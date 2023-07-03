// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ZeroMQWrapper/IPoller.h"

#include <gmock/gmock.h>

class MockPoller : public Common::ZeroMQWrapper::IPoller
{
public:
    MOCK_METHOD(poll_result_t, poll, (const std::chrono::milliseconds& timeout), (override));
    MOCK_METHOD(void, addEntry, (Common::ZeroMQWrapper::IHasFD & entry, PollDirection directionMask), (override));
    MOCK_METHOD(Common::ZeroMQWrapper::IHasFDPtr, addEntry, (int fd, PollDirection directionMask), (override));
};
