// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/sophosthreatdetectorimpl/ShutdownTimer.h"

#include <gmock/gmock.h>

namespace
{
    class MockShutdownTimer : public threat_scanner::IScanNotification
    {
    public:
        MOCK_METHOD(void, reset, ());
        MOCK_METHOD(time_t, timeout, ());
    };
}