/***********************************************************************************************

Copyright 2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/HealthStatusImpl/SerialiseFunctions.cpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class SerialiseFunctionsTests : public testing::Test
{
private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(SerialiseFunctionsTests, serialiseThreatDetectionPluginHealthStatusObj) // NOLINT
{
    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMap;

    pluginHealthStatusMap["AV"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMap["AV"].healthValue = 123;
    pluginHealthStatusMap["AV"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMap["AV"].displayName = "AV Plugin";

    pluginHealthStatusMap["RTD"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMap["RTD"].healthValue = 789;
    pluginHealthStatusMap["RTD"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMap["RTD"].displayName = "RTD Plugin";

    std::string jsonString = ManagementAgent::HealthStatusImpl::serialiseThreatHealth(pluginHealthStatusMap);
    ASSERT_EQ(
        jsonString,
        R"({"AV":{"displayName":"AV Plugin","healthType":4,"healthValue":123},"RTD":{"displayName":"RTD Plugin","healthType":4,"healthValue":789}})");
}

TEST_F(SerialiseFunctionsTests, deserialiseThreatDetectionPluginHealthStatusObj) // NOLINT
{
    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMapExpected;

    pluginHealthStatusMapExpected["AV"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMapExpected["AV"].healthValue = 1;
    pluginHealthStatusMapExpected["AV"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMapExpected["AV"].displayName = "AV Plugin";

    pluginHealthStatusMapExpected["RTD"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMapExpected["RTD"].healthValue = 2;
    pluginHealthStatusMapExpected["RTD"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMapExpected["RTD"].displayName = "RTD Plugin";

    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMap =
        ManagementAgent::HealthStatusImpl::deserialiseThreatHealth(
            R"({"AV":{"displayName":"AV Plugin","healthType":4,"healthValue":1},"RTD":{"displayName":"RTD Plugin","healthType":4,"healthValue":2}})");

    ASSERT_EQ(pluginHealthStatusMap, pluginHealthStatusMapExpected);
}