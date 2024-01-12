// Copyright 2021-2024 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/XmlUtilities/AttributesMap.h"
#include "ManagementAgent/HealthStatusImpl/HealthStatus.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    class HealthStatusTests : public LogInitializedTests
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

            EXPECT_EQ(itemFoundCount, expectedNumberOfInstances)
                << "name: " + valueToCompare.first + ", value: " + valueToCompare.second;
        }

        ManagementAgent::HealthStatusImpl::HealthStatus m_status;
    };
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWhenGoodServiceHealthValues)
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
    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "1"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "0"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWithOneBadServiceHealthValues)
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
    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
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

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWhenGoodThreatServiceHealthValues)
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
    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "1"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin One", "0"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Two", "0"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyWithOneBadThreatServiceHealthValues)
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
    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
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

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForServiceAndThreatServiceHealthValues)
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
    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
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

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForThreatDetectionsHealthValues)
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
    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
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

using namespace ManagementAgent::HealthStatusImpl;
namespace
{
    bool checkThreatHealthEq(const HealthStatus::HealthInfo& info, HealthStatus::healthValue_t expectedThreatHealth)
    {
        auto xmlString = info.statusXML;
        auto xmlMap = Common::XmlUtilities::parseXml(xmlString);
        auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

        for (const auto& path : xmlPaths)
        {
            const auto m = xmlMap.lookup(path);
            auto obtainedNameString = m.value("name");
            auto obtainedValue = m.value("value");
            if (obtainedNameString != "threat")
            {
                continue;
            }
            HealthStatus::healthValue_t actualThreatHealth = std::stoi(obtainedValue);
            return actualThreatHealth == expectedThreatHealth;
        }
        return false;
    }
} // namespace

TEST_F(HealthStatusTests, outbreakModeMakesHealthRed)
{
    auto status = m_status.generateHealthStatusXml();
    EXPECT_TRUE(checkThreatHealthEq(status, 1)); // Green

    m_status.setOutbreakMode(true);
    status = m_status.generateHealthStatusXml();
    EXPECT_TRUE(checkThreatHealthEq(status, 3)); // Red

    m_status.setOutbreakMode(false);
    status = m_status.generateHealthStatusXml();
    EXPECT_TRUE(checkThreatHealthEq(status, 1)); // Green
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForMultipleValuesForTypesAndServiceHealth)
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

    std::string expectedXml = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="admin" value="1" /><item name="service" value="3" ><detail name="Test Plugin Service And Threat One" value="2" /><detail name="Test Plugin Service One" value="2" /><detail name="Test Plugin Service Two" value="1" /></item><item name="threatService" value="3" ><detail name="Test Plugin Service And Threat One" value="2" /><detail name="Test Plugin Threat One" value="2" /><detail name="Test Plugin Threat Two" value="1" /></item><item name="threat" value="3" /></health>)";

    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
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

TEST_F(HealthStatusTests, healthStatusXML_StatusMarkedAsUnchangedWhenStatusNotChanged)
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Test Plugin One";
    pluginStatusOne.activeHeartbeat = true;
    pluginStatusOne.activeHeartbeatUtmId = "some-random-utm-id-0";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusTwo;
    pluginStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusTwo.healthValue = 0;
    pluginStatusTwo.displayName = "Test Plugin Two";
    pluginStatusTwo.activeHeartbeat = true;
    pluginStatusTwo.activeHeartbeatUtmId = "some-random-utm-id-0";

    m_status.addPluginHealth("testpluginOne", pluginStatusOne);
    m_status.addPluginHealth("testpluginTwo", pluginStatusTwo);
    ManagementAgent::HealthStatusImpl::HealthStatus::HealthInfo xmlStatus1 = m_status.generateHealthStatusXml();
    EXPECT_TRUE(xmlStatus1.hasStatusChanged);
    ManagementAgent::HealthStatusImpl::HealthStatus::HealthInfo xmlStatus2 = m_status.generateHealthStatusXml();
    EXPECT_FALSE(xmlStatus2.hasStatusChanged);
}

TEST_F(HealthStatusTests, healthStatusXML_StatusMarkedAsUnchangedWhenStatusReordedAndNotChanged)
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
    ManagementAgent::HealthStatusImpl::HealthStatus::HealthInfo xmlStatus1 = m_status.generateHealthStatusXml();
    EXPECT_TRUE(xmlStatus1.hasStatusChanged);
    ManagementAgent::HealthStatusImpl::HealthStatus::HealthInfo xmlStatus2 = m_status.generateHealthStatusXml();
    EXPECT_FALSE(xmlStatus2.hasStatusChanged);
}

TEST_F(HealthStatusTests, healthStatusXML_ResetThreatDetectionHealthMakesAllThreatDetectionPluginsHealthy)
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

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" "
                              "activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\">"
                              "<item name=\"health\" value=\"3\" />"
                              "<item name=\"admin\" value=\"1\" />"
                              "<item name=\"service\" value=\"3\" >"
                              "<detail name=\"Plugin With Service Health\" value=\"3\" /></item>"
                              "<item name=\"threatService\" value=\"3\" >"
                              "<detail name=\"Plugin With Threat Service Health\" value=\"3\" /></item>"
                              "<item name=\"threat\" value=\"1\" /></health>";

    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Plugin With Service Health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Plugin With Threat Service Health", "3"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_ResetThreatDetectionHealthMakesSingleThreatDetectionPluginHealthy)
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusThreatDetection;
    pluginStatusThreatDetection.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginStatusThreatDetection.healthValue = 3;     // Bad Health
    pluginStatusThreatDetection.displayName = "Single Threat Detection Plugin";

    m_status.addPluginHealth("testpluginOne", pluginStatusThreatDetection);

    m_status.resetThreatDetectionHealth();

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                              "<health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\">"
                              "<item name=\"health\" value=\"1\" />"
                              "<item name=\"admin\" value=\"1\" />"
                              "<item name=\"threat\" value=\"1\" />"
                              "</health>";

    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_ResetThreatDetectionHealthHasNoEffectIfThereAreNoThreatDetectionPlugins)
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

    m_status.addPluginHealth("testpluginServiceOne", pluginServiceStatusOne);
    m_status.addPluginHealth("testpluginServiceTwo", pluginServiceStatusTwo);
    m_status.addPluginHealth("testpluginThreatOne", pluginThreatStatusOne);
    m_status.addPluginHealth("testpluginThreatTwo", pluginThreatStatusTwo);
    m_status.addPluginHealth("testpluginServiceAndThreatTwo", pluginServiceAndThreatStatusOne);

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                              "<health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\">"
                              "<item name=\"health\" value=\"3\" />"
                              "<item name=\"admin\" value=\"1\" />"
                              "<item name=\"service\" value=\"3\" >"
                              "<detail name=\"Test Plugin Service And Threat One\" value=\"2\" />"
                              "<detail name=\"Test Plugin Service One\" value=\"2\" />"
                              "<detail name=\"Test Plugin Service Two\" value=\"1\" /></item>"
                              "<item name=\"threatService\" value=\"3\" >"
                              "<detail name=\"Test Plugin Service And Threat One\" value=\"2\" />"
                              "<detail name=\"Test Plugin Threat One\" value=\"2\" />"
                              "<detail name=\"Test Plugin Threat Two\" value=\"1\" /></item>"
                              "<item name=\"threat\" value=\"1\" /></health>";

    ManagementAgent::HealthStatusImpl::HealthStatus::HealthInfo pairResult = m_status.generateHealthStatusXml();
    bool stateChanged = pairResult.hasStatusChanged;
    std::string xmlString = pairResult.statusXML;

    EXPECT_TRUE(stateChanged);
    EXPECT_EQ(expectedXml, xmlString);
    EXPECT_TRUE(pairResult.hasOverallHealthChanged);
    EXPECT_EQ("{\"health\":3,\"service\":3,\"threat\":1,\"threatService\":3}", pairResult.healthJson);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service Two", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Threat One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Threat Two", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service And Threat One", "2"), 2);

    EXPECT_NO_THROW(m_status.resetThreatDetectionHealth());

    pairResult = m_status.generateHealthStatusXml();
    stateChanged = pairResult.hasStatusChanged;
    xmlString = pairResult.statusXML;
    EXPECT_FALSE(stateChanged);
    EXPECT_FALSE(pairResult.hasOverallHealthChanged);
    EXPECT_EQ(expectedXml, xmlString);
    EXPECT_EQ("", pairResult.healthJson);

    // Check that the XML can also be passed correctly.

    xmlMap = Common::XmlUtilities::parseXml(xmlString);
    attributes = xmlMap.lookup("health");
    xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service Two", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Threat One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Threat Two", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service And Threat One", "2"), 2);
}

TEST_F(HealthStatusTests, healthStatusXML_ResetThreatDetectionHealthHasNoEffectWhenThreatIsAlreadyHealthy)
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusThreatDetection;
    pluginStatusThreatDetection.healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginStatusThreatDetection.healthValue = 1;     // Good Health
    pluginStatusThreatDetection.displayName = "Plugin Threat Detection Started Healthy";

    m_status.addPluginHealth("testpluginOne", pluginStatusThreatDetection);

    m_status.resetThreatDetectionHealth();

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                              "<health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\">"
                              "<item name=\"health\" value=\"1\" />"
                              "<item name=\"admin\" value=\"1\" />"
                              "<item name=\"threat\" value=\"1\" /></health>";

    ManagementAgent::HealthStatusImpl::HealthStatus::HealthInfo pairResult = m_status.generateHealthStatusXml();
    bool stateChanged = pairResult.hasStatusChanged;
    std::string xmlString = pairResult.statusXML;
    EXPECT_TRUE(stateChanged);
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);

    EXPECT_NO_THROW(m_status.resetThreatDetectionHealth());

    pairResult = m_status.generateHealthStatusXml();
    stateChanged = pairResult.hasStatusChanged;
    xmlString = pairResult.statusXML;
    EXPECT_FALSE(stateChanged);
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    xmlMap = Common::XmlUtilities::parseXml(xmlString);
    attributes = xmlMap.lookup("health");
    xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "1"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "1"), 1);
}
TEST_F(HealthStatusTests, HealthStatusConstructorDestructorValidJsonTest)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return("{}"));
    EXPECT_CALL(
        *mockFileSystem,
        writeFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath(), "{}"));

    {
        ASSERT_NO_THROW(ManagementAgent::HealthStatusImpl::HealthStatus healthStatus);
    }
}

TEST_F(HealthStatusTests, HealthStatusConstructorDestructorInvalidJsonTest)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return("NOT JSON"));

    EXPECT_CALL(
        *mockFileSystem,
        writeFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath(), "{}"));

    {
        ASSERT_NO_THROW(ManagementAgent::HealthStatusImpl::HealthStatus healthStatus);
    }
}

TEST_F(HealthStatusTests, HealthStatusConstructorDestructorInvalidNoJsonFileTest)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(false));

    EXPECT_CALL(
        *mockFileSystem,
        writeFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath(), "{}"));

    {
        ASSERT_NO_THROW(ManagementAgent::HealthStatusImpl::HealthStatus healthStatus);
    }
}

TEST_F(HealthStatusTests, HealthStatusConstructorDestructorReadThrowsFileTest)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(true));

    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("can't read file")));

    EXPECT_CALL(
        *mockFileSystem,
        writeFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath(), "{}"));

    {
        ASSERT_NO_THROW(ManagementAgent::HealthStatusImpl::HealthStatus healthStatus);
    }
}

TEST_F(HealthStatusTests, HealthStatusDoesNotAddPluginHealthWithEmptyName)
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginStatusOne;
    pluginStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginStatusOne.healthValue = 0;
    pluginStatusOne.displayName = "Plugin Display Name";

    m_status.addPluginHealth("", pluginStatusOne);

    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
    std::string expectedXml = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="admin" value="1" /><item name="threat" value="1" /></health>)";
    ASSERT_EQ(xmlString, expectedXml);
    ASSERT_EQ("", m_status.generateHealthStatusXml().healthJson);
}


TEST_F(HealthStatusTests, healthStatusXML_IncludesCorrectUTMInformation)
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginOne;
    pluginOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginOne.healthValue = 1;
    pluginOne.displayName = "Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginTwo;
    pluginTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginTwo.healthValue = 3;
    pluginTwo.displayName = "Plugin Two";
    pluginTwo.activeHeartbeat = true;
    pluginTwo.activeHeartbeatUtmId = "some-random-utm-id";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginThree;
    pluginThree.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginThree.healthValue = 3;
    pluginThree.displayName = "Plugin Three";

    m_status.addPluginHealth("testpluginOne", pluginOne);
    std::string xmlString1 = m_status.generateHealthStatusXml().statusXML;

    m_status.addPluginHealth("testpluginTwo", pluginTwo);
    std::string xmlString2 = m_status.generateHealthStatusXml().statusXML;

    m_status.addPluginHealth("testpluginThree", pluginThree);
    std::string xmlString3 = m_status.generateHealthStatusXml().statusXML;


    std::string expectedXmlOne = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" "
                                 "activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\">"
                                 "<item name=\"health\" value=\"3\" />"
                                 "<item name=\"admin\" value=\"1\" />"
                                 "<item name=\"service\" value=\"3\" >"
                                 "<detail name=\"Plugin One\" value=\"1\" /></item>"
                                 "<item name=\"threat\" value=\"1\" /></health>";

    std::string expectedXmlTwo = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" "
                                  "activeHeartbeat=\"true\" activeHeartbeatUtmId=\"some-random-utm-id\">"
                                  "<item name=\"health\" value=\"3\" />"
                                  "<item name=\"admin\" value=\"1\" />"
                                  "<item name=\"service\" value=\"3\" >"
                                  "<detail name=\"Plugin One\" value=\"1\" />"
                                  "<detail name=\"Plugin Two\" value=\"3\" /></item>"
                                  "<item name=\"threat\" value=\"1\" /></health>";

    std::string expectedXmlThree = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" "
                                     "activeHeartbeat=\"true\" activeHeartbeatUtmId=\"some-random-utm-id\">"
                                     "<item name=\"health\" value=\"3\" />"
                                     "<item name=\"admin\" value=\"1\" />"
                                     "<item name=\"service\" value=\"3\" >"
                                     "<detail name=\"Plugin One\" value=\"1\" />"
                                     "<detail name=\"Plugin Three\" value=\"3\" />"
                                     "<detail name=\"Plugin Two\" value=\"3\" /></item>"
                                     "<item name=\"threat\" value=\"1\" /></health>";




    EXPECT_EQ(expectedXmlOne, xmlString1);
    EXPECT_EQ(expectedXmlTwo, xmlString2);
    EXPECT_EQ(expectedXmlThree, xmlString3);
    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString3);

    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
}

TEST_F(HealthStatusTests, healthStatusXML_CreatedCorrectlyForIsolation)
{
    ManagementAgent::PluginCommunication::PluginHealthStatus pluginServiceStatusOne;
    pluginServiceStatusOne.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginServiceStatusOne.healthValue = 2;
    pluginServiceStatusOne.isolated = false;
    pluginServiceStatusOne.displayName = "Test Plugin Service One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginServiceStatusTwo;
    pluginServiceStatusTwo.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginServiceStatusTwo.healthValue = 1;
    pluginServiceStatusTwo.isolated = true;
    pluginServiceStatusTwo.displayName = "Test Plugin Service Two";

    m_status.addPluginHealth("testpluginServiceOne", pluginServiceStatusOne);
    m_status.addPluginHealth("testpluginServiceTwo", pluginServiceStatusTwo);


    std::string expectedXml = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="admin" value="3" /><item name="service" value="3" ><detail name="Test Plugin Service One" value="2" /><detail name="Test Plugin Service Two" value="1" /></item><item name="threat" value="1" /></health>)";

    std::string xmlString = m_status.generateHealthStatusXml().statusXML;
    EXPECT_EQ(expectedXml, xmlString);

    // Check that the XML can also be passed correctly.

    Common::XmlUtilities::AttributesMap xmlMap = Common::XmlUtilities::parseXml(xmlString);
    auto attributes = xmlMap.lookup("health");
    auto xmlPaths = xmlMap.entitiesThatContainPath("health/item", true);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("health", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("service", "3"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threatService", "3"), 0);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("threat", "3"), 0);

    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service One", "2"), 1);
    xmlAttributesContainExpectedValues(xmlMap, xmlPaths, std::make_pair("Test Plugin Service Two", "1"), 1);
}