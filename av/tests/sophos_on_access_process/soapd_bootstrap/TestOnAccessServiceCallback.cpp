// Copyright 2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

#include "sophos_on_access_process/soapd_bootstrap/OnAccessServiceCallback.h"
#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryFields.h"
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
    auto expectedRes = R"sophos({"ratio-of-dropped-events":0.0,"ratio-of-scan-errors":0.0})sophos";
    EXPECT_EQ(resContent, expectedRes);
}


TEST_F(TestOnAccessServiceCallback, OnAccessTelemetryResets)
{
    TelemetryHelper::getInstance().increment("this is a test", 1ul);
    auto resContent = m_callback->getTelemetry();
    auto expectedRes1 = R"sophos({"ratio-of-dropped-events":0.0,"ratio-of-scan-errors":0.0,"this is a test":1})sophos";
    ASSERT_EQ(resContent, expectedRes1);

    auto resEmpty = m_callback->getTelemetry();
    auto expectedRes2 = R"sophos({"ratio-of-dropped-events":0.0,"ratio-of-scan-errors":0.0})sophos";
    EXPECT_EQ(resEmpty, expectedRes2);
}
