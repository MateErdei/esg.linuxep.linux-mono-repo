/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginProxy.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Process/IProcess.h>
#include <Telemetry/TelemetryImpl/PluginTelemetryReporter.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

using ::testing::Return;
class PluginTelemetryReporterTests : public ::testing::Test
{
public:
    void TearDown() override {}

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(PluginTelemetryReporterTests, getTelemetryOk) // NOLINT
{
    const std::string expectedPluginName = "fakePlugin";
    const std::string expectedPluginTelemetryJson = R"({"answer":42})";

    auto mockPluginProxy = std::make_unique<MockPluginProxy>();
    auto mockPluginProxyPtr = mockPluginProxy.get();

    Telemetry::PluginTelemetryReporter reporter(std::move(mockPluginProxy));

    EXPECT_CALL(*mockPluginProxyPtr, name()).WillRepeatedly(Return(expectedPluginName));
    EXPECT_CALL(*mockPluginProxyPtr, getTelemetry()).WillRepeatedly(Return(expectedPluginTelemetryJson));

    const std::string pluginName = reporter.getName();
    const std::string pluginTelemetryJson = reporter.getTelemetry();

    ASSERT_EQ(pluginName, expectedPluginName);
    ASSERT_TRUE(pluginTelemetryJson.find("answer") != std::string::npos);
}

TEST_F(PluginTelemetryReporterTests, getTelemetryWithoutIPCOk) // NOLINT
{
    const std::string expectedPluginName = "fakePlugin";
    const std::string expectedPluginTelemetryJson = R"({})";

    Telemetry::PluginTelemetryReporterWithoutIPC reporter(expectedPluginName);

    const std::string pluginName = reporter.getName();
    const std::string pluginTelemetryJson = reporter.getTelemetry();

    ASSERT_EQ(pluginName, expectedPluginName);
    ASSERT_EQ(pluginTelemetryJson, expectedPluginTelemetryJson);
}
