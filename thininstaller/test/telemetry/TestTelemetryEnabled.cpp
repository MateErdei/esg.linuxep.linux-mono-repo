// Copyright 2023 Sophos Limited. All rights reserved.

#include "telemetry/TelemetryEnabled.h"

#include "Common/Helpers/LogInitializedTests.h"
#include <gtest/gtest.h>

namespace
{
    class TestTelemetryEnabled : public LogInitializedTests
    {
    };
}

TEST_F(TestTelemetryEnabled, empty)
{
    EXPECT_FALSE(isTelemetryEnabled(""));
}

TEST_F(TestTelemetryEnabled, normalUrl)
{
    EXPECT_TRUE(isTelemetryEnabled("https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep"));
}

TEST_F(TestTelemetryEnabled, usUrl)
{
    EXPECT_FALSE(isTelemetryEnabled("https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.us/sophos/management/ep"));
}

TEST_F(TestTelemetryEnabled, httpUrl)
{
    EXPECT_FALSE(isTelemetryEnabled("http://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep"));
}

TEST_F(TestTelemetryEnabled, emptyHost)
{
    EXPECT_FALSE(isTelemetryEnabled("http:///sophos/management/ep"));
}
