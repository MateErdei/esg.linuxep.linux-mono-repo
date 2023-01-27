// Copyright 2023 Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

#include <gmock/gmock.h>

namespace
{
    class MockUpdateCompleteCallback : public threat_scanner::IUpdateCompleteCallback
    {
    public:
        MOCK_METHOD(void, updateComplete, (), (override));
    };
}