/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <pluginimpl/PluginAdapter.h>
#include <pluginimpl/PluginAdapter.cpp>
#include "datatypes/sophos_filesystem.h"
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

using namespace testing;
using namespace Plugin;

namespace fs = sophos_filesystem;

#define BASE "/tmp/TestPluginAdapter"

void setupFakeSophosThreatDetectorConfig()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", BASE);
    fs::path f = BASE;
    fs::create_directories(f / "chroot");
    f /= "sbin";
    fs::create_directories(f);
    f /= "sophos_threat_detector_launcher";
    std::ofstream ost(f);
    ost.close();
}
namespace
{
    class TestPluginAdapter : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
            testing::internal::CaptureStderr();

            m_queueTask = std::make_shared<QueueTask>();

            setupFakeSophosThreatDetectorConfig();
        }

        void TearDown() override
        {
            fs::remove_all("/tmp/TestPluginAdapter/");
        }

        std::string generatePolicyXML(const std::string& revID)
        {
            return Common::UtilityImpl::StringUtils::orderedStringReplace(
                    R"sophos(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="@@REV_ID@@" policyType="2"/>
</config>
)sophos", {{"@@REV_ID@@", revID}});
        }

        std::string generateStatusXML(const std::string& res, const std::string& revID)
        {
            return Common::UtilityImpl::StringUtils::orderedStringReplace(
                    R"sophos(<?xml version="1.0" encoding="utf-8"?>
<status xmlns="http://www.sophos.com/EE/EESavStatus">
  <csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="@@POLICY_COMPLIANCE@@" RevID="@@REV_ID@@" policyType="2"/>
  <upToDateState>1</upToDateState>
  <vdl-info>
    <virus-engine-version>N/A</virus-engine-version>
    <virus-data-version>N/A</virus-data-version>
    <idelist>
    </idelist>
    <ideChecksum>N/A</ideChecksum>
  </vdl-info>
  <on-access>false</on-access>
  <entity>
    <productId>SSPL-AV</productId>
    <product-version>N/A</product-version>
    <entityInfo>SSPL-AV</entityInfo>
  </entity>
</status>)sophos", {
                        {"@@POLICY_COMPLIANCE@@", res},
                        {"@@REV_ID@@", revID}
                    });
        }

        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<QueueTask> m_queueTask;
        std::shared_ptr<Plugin::PluginCallback> m_callback;
    };

    class MockBase : public Common::PluginApi::IBaseServiceApi
    {
    public:
        MOCK_CONST_METHOD1(requestPolicies, void(const std::string& appId));
        MOCK_CONST_METHOD2(sendEvent, void(const std::string& appId, const std::string& eventXml));
        MOCK_CONST_METHOD3(sendStatus, void(
        const std::string& appId,
        const std::string& statusXml,
        const std::string& statusWithoutTimestampsXml));
    };
}

ACTION_P(QueueStopTask, taskQueue) {
    taskQueue->pushStop();
}

TEST_F(TestPluginAdapter, testConstruction) //NOLINT
{
    PluginAdapter pluginAdapter(m_queueTask, std::move(m_baseService), m_callback);
}

TEST_F(TestPluginAdapter, testProcessPolicy) //NOLINT
{
    auto mockBaseService = std::make_unique<StrictMock<MockBase>>();
    MockBase* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generatePolicyXML(policy1revID);
    std::string policy2Xml = generatePolicyXML(policy2revID);

    Task policy1Task = {Task::TaskType::Policy, policy1Xml};
    Task policy2Task = {Task::TaskType::Policy, policy2Xml};
    m_queueTask->push(policy1Task);
    m_queueTask->push(policy2Task);

    std::string status1Xml = generateStatusXML("NoRef", policy1revID);
    std::string status2Xml = generateStatusXML("Same", policy2revID);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status1Xml, status1Xml)).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).WillOnce(QueueStopTask(m_queueTask));

    pluginAdapter.mainLoop();
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logs, HasSubstr("Process policy: " + policy1Xml));
    EXPECT_THAT(logs, HasSubstr("Process policy: " + policy2Xml));
    EXPECT_THAT(logs, HasSubstr("Received new policy with revision ID: 123"));
}


TEST_F(TestPluginAdapter, testProcessAction) //NOLINT
{
    PluginAdapter pluginAdapter(m_queueTask, std::move(m_baseService), m_callback);

    std::string actionXml =
            R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>)";

    Task actionTask = {Task::TaskType::Action,
                       actionXml};
    Task stopTask = {Task::TaskType::Stop, ""};

    m_queueTask->push(actionTask);
    m_queueTask->push(stopTask);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logs, HasSubstr(expectedLog));
    EXPECT_THAT(logs, HasSubstr("Starting Scan Now scan"));
}

TEST_F(TestPluginAdapter, testProcessActionMalformed) //NOLINT
{
    PluginAdapter pluginAdapter(m_queueTask, std::move(m_baseService), m_callback);

    std::string actionXml =
            R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="NONE" id="" subtype="MALFORMED" replyRequired="0"/>)";

    Task actionTask = {Task::TaskType::Action,
                       actionXml};
    Task stopTask = {Task::TaskType::Stop, ""};

    m_queueTask->push(actionTask);
    m_queueTask->push(stopTask);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logs, HasSubstr(expectedLog));
    EXPECT_THAT(logs, Not(HasSubstr("Starting Scan Now scan")));
}

TEST_F(TestPluginAdapter, testProcessThreatReport) //NOLINT
{
    auto mockBaseService = std::make_unique<StrictMock<MockBase> >();
    MockBase* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string threatDetectedXML = R"sophos(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
                     <notification xmlns="http://www.sophos.com/EE/Event"
                               description="Virus/spyware eicar has been detected in path/to/threat"
                               type="sophos.mgt.msg.event.threat"
                               timestamp="123">

                     <user userId="User"
                               domain="local"/>
                     <threat  type="1"
                               name="eicar"
                               scanType="201"
                               status="50"
                               id="1"
                               idSource="1">

                               <item file="threat"
                                      path="path/to/threat"/>
                               <action action="104"/>
                     </threat>
                     </notification>
            )sophos";

    EXPECT_CALL(*mockBaseServicePtr, sendEvent("SAV", threatDetectedXML));

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback);
    pluginAdapter.processThreatReport(threatDetectedXML);
    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending threat detection notification to central: ";
    expectedLog.append(threatDetectedXML);
    std::string logs = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logs, HasSubstr(expectedLog));
}