// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/sophosthreatdetectorimpl/ShutdownTimer.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    class MockShutdownTimer : public threat_scanner::IScanNotification
    {
    public:
        MockShutdownTimer(time_t timeout = 0)
        {
            m_timeout = timeout;

            ON_CALL(*this, timeout).WillByDefault(Return(m_timeout));
        }

        MOCK_METHOD(void, reset, ());
        MOCK_METHOD(time_t, timeout, ());
    private:
        time_t m_timeout;
    };
}