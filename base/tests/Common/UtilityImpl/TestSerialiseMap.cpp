///******************************************************************************************************
//
//Copyright 2021, Sophos Limited.  All rights reserved.
//
//******************************************************************************************************/
//#include <Common/UtilityImpl/MapSerialiser.cpp>
//#include <gtest/gtest.h>
//
//using namespace Common::UtilityImpl;
//
//
//enum class HealthType
//{
//    NONE,
//    SERVICE,
//    THREAT_SERVICE,
//    SERVICE_AND_THREAT,
//    THREAT_DETECTION
//};
//
//struct PluginHealthStatus
//{
//    PluginHealthStatus() : healthType(HealthType::NONE), healthValue(0)
//    {
//    }
//
//    HealthType healthType;
//    std::string displayName;
//    unsigned int healthValue;
//};
//
//
//TEST(MapSerialiserTests, test1) // NOLINT
//{
//    std::map<std::string, std::string> m1;
//    m1["asdasd"] = "";
//    std::string mapJson = serialiseMap(m1);
//    auto m2 = deserialiseMap(mapJson);
////    ASSERT_EQ(m1, m2);
//
//}
//
//TEST(MapSerialiserTests, test2) // NOLINT
//{
//
//    std::map<std::string, PluginHealthStatus> pluginThreatDetectionHealth;
//    std::string mapJson = serialiseMap(pluginThreatDetectionHealth);
//    auto m2 = deserialiseMap(mapJson);
////    ASSERT_EQ(pluginThreatDetectionHealth, m2);
//
//}
//
