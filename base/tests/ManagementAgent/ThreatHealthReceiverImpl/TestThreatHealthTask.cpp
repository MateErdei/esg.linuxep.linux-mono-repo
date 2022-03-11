///******************************************************************************************************
//
// Copyright 2021, Sophos Limited.  All rights reserved.
//
//******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <ManagementAgent/ThreatHealthReceiverImpl/ThreatHealthTask.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

class TestThreatHealthTask : public LogInitializedTests
{
};

TEST_F(TestThreatHealthTask, Construction) // NOLINT
{
    auto healthStatusSharedObj = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    ASSERT_NO_THROW // NOLINT
        (ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask task("PluginName", 123, healthStatusSharedObj));
}

TEST_F(TestThreatHealthTask, checkTaskAddsThreatHealth) // NOLINT
{
    auto healthStatusSharedObj = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask task("PluginName", 123, healthStatusSharedObj);

    auto generatedXmlAndSuccessBoolPairBefore = healthStatusSharedObj->generateHealthStatusXml();
    ASSERT_NO_THROW(task.run()); // NOLINT
    auto generatedXmlAndSuccessBoolPairAfter = healthStatusSharedObj->generateHealthStatusXml();

    std::string expectedXmlBefore =
        R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="threat" value="1" /></health>)";
    ASSERT_EQ(generatedXmlAndSuccessBoolPairBefore.hasStatusChanged, true);
    ASSERT_EQ(generatedXmlAndSuccessBoolPairBefore.statusXML, expectedXmlBefore);

    std::string expectedXmlAfter =
        R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="123" /><item name="threat" value="123" /></health>)";
    ASSERT_EQ(generatedXmlAndSuccessBoolPairAfter.hasStatusChanged, true);
    ASSERT_EQ(generatedXmlAndSuccessBoolPairAfter.statusXML, expectedXmlAfter);
}

TEST_F(TestThreatHealthTask, checkTwoTasksCanUpdateHealthStatus) // NOLINT
{
    auto healthStatusSharedObj = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask task1("PluginName", 1, healthStatusSharedObj);
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask task2("A Different Plugin", 2, healthStatusSharedObj);

    auto generatedXmlAndSuccessBoolPairBefore = healthStatusSharedObj->generateHealthStatusXml();
    ASSERT_NO_THROW(task1.run()); // NOLINT
    ASSERT_NO_THROW(task2.run()); // NOLINT
    auto generatedXmlAndSuccessBoolPairAfter = healthStatusSharedObj->generateHealthStatusXml();

    std::string expectedXmlBefore =
        R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="threat" value="1" /></health>)";
    ASSERT_EQ(generatedXmlAndSuccessBoolPairBefore.hasStatusChanged, true);
    ASSERT_EQ(generatedXmlAndSuccessBoolPairBefore.statusXML, expectedXmlBefore);

    std::string expectedXmlAfter =
        R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="2" /><item name="threat" value="2" /></health>)";
    ASSERT_EQ(generatedXmlAndSuccessBoolPairAfter.hasStatusChanged, true);
    ASSERT_EQ(generatedXmlAndSuccessBoolPairAfter.statusXML, expectedXmlAfter);
}

TEST_F(TestThreatHealthTask, checkTaskWorksWithEmptyPluginName) // NOLINT
{
    auto healthStatusSharedObj = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask task("", 0, healthStatusSharedObj);

    auto generatedXmlAndChangeIndicatorBoolPairBefore = healthStatusSharedObj->generateHealthStatusXml();
    ASSERT_NO_THROW(task.run()); // NOLINT
    auto generatedXmlAndChangeIndicatorBoolPairAfter = healthStatusSharedObj->generateHealthStatusXml();

    // Output XML should be unchanged
    std::string expectedXmlBefore =
        R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="threat" value="1" /></health>)";
    ASSERT_EQ(generatedXmlAndChangeIndicatorBoolPairBefore.hasStatusChanged, true);
    ASSERT_EQ(generatedXmlAndChangeIndicatorBoolPairBefore.statusXML, expectedXmlBefore);
    ASSERT_EQ(generatedXmlAndChangeIndicatorBoolPairAfter.hasStatusChanged, false);
    ASSERT_EQ(generatedXmlAndChangeIndicatorBoolPairAfter.statusXML, expectedXmlBefore);
}

TEST_F(TestThreatHealthTask, checkTaskDoesNotThrowWhenUninitialisedHealthStatusObjIsUsed) // NOLINT
{
    std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj;
    ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask task("", 0, healthStatusSharedObj);
    ASSERT_NO_THROW(task.run()); // NOLINT
}