// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IScanNotification.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    class MockShutdownTimer : public threat_scanner::IScanNotification
    {
        public:
            MOCK_METHOD(void, reset, ());
            MOCK_METHOD(time_t, timeout, ());
    };
}