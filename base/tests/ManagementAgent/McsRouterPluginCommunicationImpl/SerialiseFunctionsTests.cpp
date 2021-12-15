/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <ManagementAgent/HealthStatusImpl/SerialiseFunctions.cpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(SerialiseFunctionsTests, test1)
{

    nlohmann::json pluginHealthStatusJsonObj;
    pluginHealthStatusJsonObj["EDR"]["displayName"] = "EDR Plugin";
    pluginHealthStatusJsonObj["EDR"]["healthValue"] = "1";
    pluginHealthStatusJsonObj["EDR"]["healthType"] = 0;


    auto blah = pluginHealthStatusJsonObj.dump();
    std::cout << blah;
    return;

    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMap;
    pluginHealthStatusMap["AV"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMap["AV"].healthValue = 1;
    pluginHealthStatusMap["AV"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMap["AV"].displayName = "AV Plugin";
    std::string jsonString = ManagementAgent::HealthStatusImpl::serialiseThreatHealth(pluginHealthStatusMap);
    ASSERT_EQ(jsonString, "asdasd");

}
