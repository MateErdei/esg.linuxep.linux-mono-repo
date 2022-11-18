// Copyright 2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

#include "sophos_on_access_process/soapd_bootstrap/OnAccessServiceCallback.h"
#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryUtility.h"

#include "common/LogInitializedTests.h"


#include <gtest/gtest.h>

#include <thread>

using namespace sophos_on_access_process::service_callback;
using namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry;
using namespace Common::Telemetry;
using namespace testing;

class TestOnAccessServiceCallback : public LogInitializedTests
{
protected:
    void SetUp() override
    {
        auto telemetryUtilty = std::make_shared<OnAccessTelemetryUtility>();
        m_callback = std::make_unique<OnAccessServiceCallback>(telemetryUtilty);
    }
    std::unique_ptr<OnAccessServiceCallback> m_callback;
};

TEST_F(TestOnAccessServiceCallback, OnAccessTelemetryGetsTelemetryFromTelemetryHelper)
{
    auto resContent = m_callback->getTelemetry();
    ASSERT_EQ(resContent, "{\"Ratio-of-Dropped-Events\":0.0}");
}


TEST_F(TestOnAccessServiceCallback, OnAccessTelemetryResets)
{
    TelemetryHelper::getInstance().increment("This is a test", 1ul);
    auto resContent = m_callback->getTelemetry();
    ASSERT_EQ(resContent, "{\"Ratio-of-Dropped-Events\":0.0,\"This is a test\":1}");
    auto resEmpty = m_callback->getTelemetry();
    ASSERT_EQ(resEmpty, "{\"Ratio-of-Dropped-Events\":0.0}");
}
