/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/


#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/HealthStatus.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class HealthStatusTests : public testing::Test
{
protected:
    void xmlAttributesContainExpectedValues(
        const Common::XmlUtilities::AttributesMap& xmlMap,
        const std::vector<std::string>& xmlPaths,
        const std::pair<std::string, std::string>& valueToCompare,
        int expectedNumberOfInstances)
    {
        int itemFoundCount = 0;

        SCOPED_TRACE("name: " + valueToCompare.first + ", value: " + valueToCompare.second);

        for (auto& path : xmlPaths)
        {
            auto attributesList = xmlMap.lookup(path);
            auto obtainedNameString = attributesList.value("name");
            auto obtainedValue = attributesList.value("value");
            if (obtainedNameString == valueToCompare.first && obtainedValue == valueToCompare.second)
            {
                itemFoundCount++;
            }
        }

        EXPECT_EQ(itemFoundCount, expectedNumberOfInstances) << "name: " + valueToCompare.first + ", value: " + valueToCompare.second;
    }

    ManagementAgent::McsRouterPluginCommunicationImpl::HealthStatus m_status;

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWhenGoodServiceHealthValues) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusTwo.healthValue = 0;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::string xmlString = m_status.generateHealthStatusXml().second;
    Common::XmlUtilities::AttributesMap xmlMap =
        Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "1"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "0"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWithOneBadServiceHealthValues) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusTwo.healthValue = 1;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::string xmlString = m_status.generateHealthStatusXml().second;
    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "1"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "1"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWhenGoodThreatServiceHealthValues) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginStatusTwo.healthValue = 0;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::string xmlString = m_status.generateHealthStatusXml().second;
    Common::XmlUtilities::AttributesMap xmlMap =
        Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "1"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "0"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWithOneBadThreatServiceHealthValues) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginStatusTwo.healthValue = 1;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::string xmlString = m_status.generateHealthStatusXml().second;
    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "1"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "1"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForServiceAndThreatServiceHealthValues) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE_AND_THREAT;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE_AND_THREAT;
    pluginStatusTwo.healthValue = 1;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::string xmlString = m_status.generateHealthStatusXml().second;
    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 2);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "1"), 2);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForThreatDetectionsHealthValues) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginStatusTwo.healthValue = 1;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::string xmlString = m_status.generateHealthStatusXml().second;
    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "0"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "0"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "1"), 0);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForMultipleValuesForTypesAndServiceHealth) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginServiceStatusOne;
    pluginServiceStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginServiceStatusOne.healthValue = 2;
    pluginServiceStatusOne.displayName = "Test Plugin Service One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginServiceStatusTwo;
    pluginServiceStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginServiceStatusTwo.healthValue = 1;
    pluginServiceStatusTwo.displayName = "Test Plugin Service Two";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginThreatStatusOne;
    pluginThreatStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginThreatStatusOne.healthValue = 2;
    pluginThreatStatusOne.displayName = "Test Plugin Threat One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginThreatStatusTwo;
    pluginThreatStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginThreatStatusTwo.healthValue = 1;
    pluginThreatStatusTwo.displayName = "Test Plugin Threat Two";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginServiceAndThreatStatusOne;
    pluginServiceAndThreatStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE_AND_THREAT;
    pluginServiceAndThreatStatusOne.healthValue = 2;
    pluginServiceAndThreatStatusOne.displayName = "Test Plugin Service And Threat One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginDetectionStatusOne;
    pluginDetectionStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginDetectionStatusOne.healthValue = 1;
    pluginDetectionStatusOne.displayName = "Test Plugin Detection One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginDetectionStatusTwo;
    pluginDetectionStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginDetectionStatusTwo.healthValue = 3;
    pluginDetectionStatusTwo.displayName = "Test Plugin Detection Two";

    m_status.addPluginHealth("testpluginServiceOne", pluginServiceStatusOne);
    m_status.addPluginHealth("testpluginServiceTwo", pluginServiceStatusTwo);
    m_status.addPluginHealth("testpluginThreatOne", pluginThreatStatusOne);
    m_status.addPluginHealth("testpluginThreatTwo", pluginThreatStatusTwo);
    m_status.addPluginHealth("testpluginServiceAndThreatTwo", pluginServiceAndThreatStatusOne);
    m_status.addPluginHealth("testpluginDetectionOne", pluginDetectionStatusOne);
    m_status.addPluginHealth("testpluginDetectionTwo", pluginDetectionStatusTwo);

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\"><item name=\"health\" value=\"3\" /><item name=\"service\" value=\"3\" ><detail name=\"Test Plugin Service And Threat One\" value=\"2\" /><detail name=\"Test Plugin Service One\" value=\"2\" /><detail name=\"Test Plugin Service Two\" value=\"1\" /></item><item name=\"threatService\" value=\"3\" ><detail name=\"Test Plugin Service And Threat One\" value=\"2\" /><detail name=\"Test Plugin Threat One\" value=\"2\" /><detail name=\"Test Plugin Threat Two\" value=\"1\" /></item><item name=\"threat\" value=\"3\" /></health>";

    std::string xmlString = m_status.generateHealthStatusXml().second;
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "3"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service Two", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Threat One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Threat Two", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service And Threat One", "2"), 2);
}

TEST_F(HealthStatusTests, healthStatusXML_StatusMarkedAsUnchangedWhenStatusNotChanged) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusTwo.healthValue = 0;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::pair<bool, std::string> xmlStatus1 = m_status.generateHealthStatusXml();
    EXPECT_TRUE(xmlStatus1.first);
    std::pair<bool, std::string> xmlStatus2 = m_status.generateHealthStatusXml();
    EXPECT_FALSE(xmlStatus2.first);
}

TEST_F(HealthStatusTests, healthStatusXML_StatusMarkedAsUnchangedWhenStatusReordedAndNotChanged) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusTwo.healthValue = 0;
    pluginStatusTwo.displayName = "Test Plugin Two";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    std::pair<bool, std::string> xmlStatus1 = m_status.generateHealthStatusXml();
    EXPECT_TRUE(xmlStatus1.first);
    std::pair<bool, std::string> xmlStatus2 = m_status.generateHealthStatusXml();
    EXPECT_FALSE(xmlStatus2.first);
}

TEST_F(HealthStatusTests, healthStatusXML_ResetThreatDetectionHealthMakesAllThreatDetectionPluginsHealthy) // NOLINT
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusJustService;
    pluginStatusJustService.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusJustService.healthValue = 3;  // Bad Health
    pluginStatusJustService.displayName = "Plugin With Service Health";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusJustThreat;
    pluginStatusJustThreat.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_SERVICE;
    pluginStatusJustThreat.healthValue = 3;    // Bad Health
    pluginStatusJustThreat.displayName = "Plugin With Threat Service Health";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusThreatDetectionOne;
    pluginStatusThreatDetectionOne.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginStatusThreatDetectionOne.healthValue = 1;     // Good Health
    pluginStatusThreatDetectionOne.displayName = "Plugin Threat Detection Started Healthy";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusThreatDetectionTwo;
    pluginStatusThreatDetectionTwo.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginStatusThreatDetectionTwo.healthValue = 3;     // Bad Health
    pluginStatusThreatDetectionTwo.displayName = "Plugin Threat Detection Started Bad";

    m_status.addPluginHealth("testpluginOne", pluginStatusJustService);
    m_status.addPluginHealth("testpluginTwo", pluginStatusJustThreat);
    m_status.addPluginHealth("testpluginThree", pluginStatusThreatDetectionOne);
    m_status.addPluginHealth("testpluginFour", pluginStatusThreatDetectionTwo);

    m_status.resetThreatDetectionHealth();

    // TODO: LINUXDAR-3796 expected status will change once we're reporting Threat Detection Status.
    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" "
                              "activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\">"
                              "<item name=\"health\" value=\"3\" />"
                              "<item name=\"service\" value=\"3\" >"
                              "<detail name=\"Plugin With Service Health\" value=\"3\" /></item>"
                              "<item name=\"threatService\" value=\"3\" >"
                              "<detail name=\"Plugin With Threat Service Health\" value=\"3\" /></item>"
                              "<item name=\"threat\" value=\"3\" /></health>";

    std::string xmlString = m_status.generateHealthStatusXml().second;
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "3"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Plugin With Service Health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Plugin With Threat Service Health", "3"), 1);
}
