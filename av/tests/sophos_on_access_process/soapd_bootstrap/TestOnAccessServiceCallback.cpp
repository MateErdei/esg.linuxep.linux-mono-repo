// Copyright 2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

#include "sophos_on_access_process/soapd_bootstrap/OnAccessServiceCallback.h"

#include "common/LogInitializedTests.h"


#include <gtest/gtest.h>

#include <thread>

using namespace sophos_on_access_process::service_callback;
using namespace testing;

class TestOnAccessServiceCallback : public LogInitializedTests
{
protected:
    void SetUp() override
    {
    }
};


TEST_F(TestOnAccessServiceCallback, OnAccessTelemetryResets)
{
    auto callback = sophos_on_access_process::service_callback::OnAccessServiceCallback {};

    Common::Telemetry::TelemetryHelper::getInstance().increment("This is a test", 1ul);
    auto resContent = callback.getTelemetry();
    ASSERT_EQ(resContent, "{\"This is a test\":1}");
    auto resEmpty = callback.getTelemetry();
    ASSERT_EQ(resEmpty, "{}");
}
