// Copyright 2020-2023 Sophos Limited. All rights reserved.

#define PLUGIN_INTERNAL public

#include "PluginMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/DetectionReporter.h"
#include "pluginimpl/Logger.h"
#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/StringUtils.h"
#include "tests/common/Common.h"
#include "tests/common/SetupFakePluginDir.h"
#include "tests/scan_messages/SampleThreatDetected.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockApiBaseServices.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ZeroMQWrapper/IIPCException.h"
#include "Common/ZeroMQWrapper/ISocketSubscriber.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <future>

using namespace testing;
using namespace Plugin;
using namespace scan_messages;
using namespace common::CentralEnums;

namespace fs = sophos_filesystem;

namespace
{
    class TestPluginAdapter : public PluginMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_tempDir = setupFakePluginDir();
            m_threatEventPublisherSocketPath = pluginInstall() / "threatEventPublisherSocket";

            Common::Telemetry::TelemetryHelper::getInstance().reset();

            m_taskQueue = std::make_shared<TaskQueue>();
            m_callback = std::make_shared<Plugin::PluginCallback>(m_taskQueue);
        }

        static std::string generatePolicyXML(
            const std::string& revID,
            const std::string& policyID = "2",
            const std::string& sxl = "false")
        {
            return Common::UtilityImpl::StringUtils::orderedStringReplace(
                R"sophos(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="@@REV_ID@@" policyType="@@POLICY_ID@@"/>
     <detectionFeedback>
        <sendData>@@SXL@@</sendData>
        <sendFiles>false</sendFiles>
        <onDemandEnable>true</onDemandEnable>
      </detectionFeedback>
</config>
)sophos",
                { { "@@REV_ID@@", revID }, { "@@POLICY_ID@@", policyID }, { "@@SXL@@", sxl } });
        }

        static std::string generateUpdatePolicyXML(const std::string& revID, const std::string& policyID = "1")
        {
            return Common::UtilityImpl::StringUtils::orderedStringReplace(
                R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="@@REV_ID@@" policyType="@@POLICY_ID@@"/>
  <AUConfig>
    <primary_location>
      <server UserPassword="A" UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos",
                { { "@@REV_ID@@", revID }, { "@@POLICY_ID@@", policyID } });
        }

        static std::string generateStatusXML(const std::string& res, const std::string& revID)
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
    <product-version>unknown</product-version>
    <entityInfo>SSPL-AV</entityInfo>
  </entity>
</status>)sophos",
                { { "@@POLICY_COMPLIANCE@@", res }, { "@@REV_ID@@", revID } });
        }

        static void expectedDefaultPolicyRequests(StrictMock<MockApiBaseServices>& mock)
        {
            for (const auto& policy : PluginAdapter::m_requested_policies)
            {
                EXPECT_CALL(mock, requestPolicies(policy)).Times(1);
            }
        }

        std::unique_ptr<Tests::TempDir> m_tempDir;
        std::shared_ptr<TaskQueue> m_taskQueue;
        std::shared_ptr<Plugin::PluginCallback> m_callback;
        fs::path m_threatEventPublisherSocketPath;
    };
} // namespace

ACTION_P(QueueStopTask, taskQueue)
{
    taskQueue->pushStop();
}

TEST_F(TestPluginAdapter, testConstruction)
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
}

TEST_F(TestPluginAdapter, testMainLoop)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);
#ifdef ENABLE_CORE_POLICY
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("CORE")).Times(1);
#endif
#ifdef ENABLE_CORC_POLICY
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("CORC")).Times(1);
#endif
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillOnce(QueueStopTask(m_taskQueue));
    pluginAdapter.mainLoop();
}

TEST_F(TestPluginAdapter, testRequestPoliciesThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();

    Common::PluginApi::ApiException ex{ "dummy error" };
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, requestPolicies(_))
        .Times(PluginAdapter::m_requested_policies.size())
        .WillRepeatedly(Throw(ex));

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    Task policyTask = { .taskType = Task::TaskType::Policy, .Content = policyXml, .appId = "SAV" };
    m_taskQueue->push(policyTask);

    m_taskQueue->pushStop();
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received SAV policy"));
    EXPECT_TRUE(appenderContains("Failed to request SAV policy at startup (dummy error)"));
    EXPECT_TRUE(appenderContains("Failed to request ALC policy at startup (dummy error)"));
    EXPECT_TRUE(appenderContains("Failed to request FLAGS policy at startup (dummy error)"));
}

TEST_F(TestPluginAdapter, testProcessPolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink(_)).WillRepeatedly(::testing::Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, isFile(_)).WillOnce(Return(false));

    fs::path testDir = tmpdir();
    const std::string susiStartupSettingsPath = testDir / "var/susi_startup_settings.json";
    const std::string susiStartupSettingsChrootPath = std::string(testDir / "chroot") + susiStartupSettingsPath;
    Common::FileSystem::IFileSystemException ex(
        "Error, Failed to read file: '" + susiStartupSettingsPath + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, readFile(_)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(_, _)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFileAtomically(_, _, _, _)).WillRepeatedly(Throw(ex));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    auto* mockBaseServicePtr = mockBaseService.get();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy3revID = "12345678903";
    std::string policy1Xml = generatePolicyXML(policy1revID, "2", "true");
    std::string policy2Xml = generatePolicyXML(policy2revID);
    std::string policy3Xml = generatePolicyXML(policy3revID, "2", "true");

    Task policy1Task = { .taskType = Task::TaskType::Policy, .Content = policy1Xml, .appId = "SAV" };
    Task policy2Task = { .taskType = Task::TaskType::Policy, .Content = policy2Xml, .appId = "SAV" };
    Task policy3Task = { .taskType = Task::TaskType::Policy, .Content = policy3Xml, .appId = "SAV" };
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy2Task);
    m_taskQueue->push(policy3Task);

    std::string initialStatusXml = generateStatusXML("NoRef", "");
    std::string status1Xml = generateStatusXML("Same", policy1revID);
    std::string status2Xml = generateStatusXML("Same", policy2revID);
    std::string status3Xml = generateStatusXML("Same", policy3revID);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status1Xml, status1Xml)).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status3Xml, status3Xml)).WillOnce(QueueStopTask(m_taskQueue));
    EXPECT_EQ(m_callback->getStatus("SAV").statusXml, initialStatusXml);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    expectedDefaultPolicyRequests(*mockBaseServicePtr);

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received SAV policy"));
    EXPECT_TRUE(appenderContains("Processing SAV policy: " + policy1Xml));
    EXPECT_TRUE(appenderContains("Processing SAV policy: " + policy2Xml));
    EXPECT_TRUE(appenderContains("Received new policy with revision ID: 123"));
#ifdef USE_SXL_ENABLE_FROM_CORC_POLICY
    // We now see all of the reload events
    EXPECT_EQ(appenderCount("Processing request to reload sophos threat detector"), 3);
    // 1st and 3rd policies will required a reload - but the reload will be delayed till
    // the 3rd policy has been processed
    EXPECT_EQ(appenderCount("Requesting scan monitor to reload susi"), 1);
#else
    // We now see all of the restart events
    EXPECT_EQ(appenderCount("Processing request to restart sophos threat detector"), 3);
    // 1st and 3rd policies will required a restart - but the restart will be delayed till
    // the 3rd policy has been processed
    EXPECT_EQ(appenderCount("Requesting scan monitor to restart threat detector"), 1);
#endif
}

TEST_F(TestPluginAdapter, testProcessPolicy_ignoresDuplicates)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink(_)).WillRepeatedly(::testing::Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, isFile(_)).WillOnce(Return(false));

    fs::path testDir = tmpdir();
    const std::string susiStartupSettingsPath = testDir / "var/susi_startup_settings.json";
    const std::string susiStartupSettingsChrootPath = std::string(testDir / "chroot") + susiStartupSettingsPath;
    Common::FileSystem::IFileSystemException ex(
        "Error, Failed to read file: '" + susiStartupSettingsPath + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, readFile(_)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(_, _)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFileAtomically(_, _, _, _)).WillRepeatedly(Throw(ex));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    auto* mockBaseServicePtr = mockBaseService.get();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policyRevID1 = "12345678901";
    std::string policyRevID2 = "12345678902";
    std::string policy1Xml = generatePolicyXML(policyRevID1);
    std::string policy2Xml = generatePolicyXML(policyRevID2);

    Task policy1Task = { .taskType = Task::TaskType::Policy, .Content = policy1Xml, .appId = "SAV" };
    Task policy2Task = { .taskType = Task::TaskType::Policy, .Content = policy2Xml, .appId = "SAV" };
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy2Task);

    std::string initialStatusXml = generateStatusXML("NoRef", "");
    std::string status1Xml = generateStatusXML("Same", policyRevID1);
    std::string status2Xml = generateStatusXML("Same", policyRevID2);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status1Xml, status1Xml)).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).WillOnce(QueueStopTask(m_taskQueue));
    EXPECT_EQ(m_callback->getStatus("SAV").statusXml, initialStatusXml);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    expectedDefaultPolicyRequests(*mockBaseServicePtr);

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received SAV policy", 2));
    EXPECT_TRUE(appenderContains("Processing SAV policy: " + policy1Xml, 1));
    EXPECT_TRUE(appenderContains("Processing SAV policy: " + policy2Xml, 1));
    EXPECT_TRUE(appenderContains("Policy with app id SAV unchanged, will not be processed", 2));
}

TEST_F(TestPluginAdapter, testWaitForTheFirstPolicyReturnsEmptyPolicyOnInvalidPolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    expectedDefaultPolicyRequests(*mockBaseService);

    const std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    const Task policyTask = { .taskType = Task::TaskType::Policy, .Content = policyXml, .appId = "unknown" };
    m_taskQueue->push(policyTask);


    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 1);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    // check these first to ensure the timing is correct
    std::this_thread::sleep_for(500ms);
    EXPECT_FALSE(appenderContains("ALC policy has not been sent to the plugin"));
    EXPECT_FALSE(appenderContains("SAV policy has not been sent to the plugin"));

    EXPECT_TRUE(waitForLog("Ignoring unknown policy with APPID: unknown"));

    EXPECT_TRUE(waitForLog("ALC policy has not been sent to the plugin", 1000ms));
    EXPECT_TRUE(waitForLog("SAV policy has not been sent to the plugin"));

    m_taskQueue->pushStop();

    pluginThread.join();
}

TEST_F(TestPluginAdapter, testProcessPolicy_ignoresPolicyWithWrongID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    expectedDefaultPolicyRequests(*mockBaseService);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generatePolicyXML(policy1revID, "1");
    std::string policy2Xml = generatePolicyXML(policy2revID);

    Task policy1Task = { .taskType = Task::TaskType::Policy, .Content = policy1Xml, .appId = "SAV" };
    Task policy2Task = { .taskType = Task::TaskType::Policy, .Content = policy2Xml, .appId = "SAV" };
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy2Task);

    std::string initialStatusXml = generateStatusXML("NoRef", "");
    std::string status1Xml = generateStatusXML("Same", policy1revID);
    std::string status2Xml = generateStatusXML("Same", policy2revID);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).WillOnce(QueueStopTask(m_taskQueue));

    EXPECT_EQ(m_callback->getStatus("SAV").statusXml, initialStatusXml);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received SAV policy"));
    EXPECT_TRUE(appenderContains("Processing SAV policy: " + policy2Xml));
    EXPECT_TRUE(appenderContains("Ignoring unknown policy with APPID: SAV, content:"));
    EXPECT_TRUE(appenderContains("Received new policy with revision ID: 123"));
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    expectedDefaultPolicyRequests(*mockBaseService);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink(_)).WillRepeatedly(::testing::Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, isFile(_)).WillOnce(Return(false));

    fs::path testDir = pluginInstall();
    const std::string expectedMd5 = "a1c0f318e58aad6bf90d07cabda54b7d"; // md5(md5("B:A"))
    const std::string customerIdFilePath1 = testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(testDir / "chroot") + customerIdFilePath1;
    Common::FileSystem::IFileSystemException ex(
        "Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(Plugin::getPersistThreatDatabaseFilePath(), "{}")).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).WillOnce(QueueStopTask(m_taskQueue));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policyRevID = "12345678901";
    std::string policyXml = generateUpdatePolicyXML(policyRevID);

    Task policyTask = { .taskType = Task::TaskType::Policy, .Content = policyXml, .appId = "ALC" };

    m_taskQueue->push(policyTask);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received ALC policy"));
    EXPECT_TRUE(appenderContains("Processing ALC policy: " + policyXml));
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicyThrowsIfInvalidXML)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger threadRunnerLogger = Common::Logging::getInstance("Common");
    threadRunnerLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string brokenPolicyXml = Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"sophos(<?xml version="1.0"?>
        <config xmlns="http://www.sophos.com/EE/EESavConfiguration">
          <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="@@REV_ID@@" policyType="@@POLICY_ID@@"/>
             <detectionFeedback>
                <sendData>@@SXL@@</sendData>
                <sendFiles>false</sendFiles>
                true</onDemandEnable>
              </detectionFeedback>
        </config>
        )sophos",
        { { "@@REV_ID@@", "12345678901" }, { "@@POLICY_ID@@", "2" }, { "@@SXL@@", "false" } });

    Task policyTask = { .taskType = Task::TaskType::Policy, .Content = brokenPolicyXml, .appId = "SAV" };
    m_taskQueue->push(policyTask);

    EXPECT_TRUE(waitForLog("Exception encountered while parsing SAV policy XML: Error parsing xml", 500ms));

    Task stopTask = { Task::TaskType::Stop, "" };
    m_taskQueue->push(stopTask);
    EXPECT_TRUE(waitForLog("Stop task received"));
    EXPECT_FALSE(appenderContains("SAV policy received for the first time."));

    pluginThread.join();
}

TEST_F(TestPluginAdapter, testPluginAdaptorDoesntRestartThreatDetectorWithInvalidPolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger threadRunnerLogger = Common::Logging::getInstance("Common");
    threadRunnerLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string invalidPolicyXml =
        R"sophos(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
   <scanSet>
        <!-- if {{scheduledScanEnabled}} -->
        <scan>
            <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
        <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>{{day}}</day>
        </daySet>
        <timeSet>
            <time>{{scheduledScanTime}}</time>
        </timeSet>
        </schedule>
        <settings>
        <scanObjectSet>
        <CDDVDDrives>false</CDDVDDrives>
        <hardDrives>true</hardDrives>
        <networkDrives>false</networkDrives>
        <removableDrives>false</removableDrives>
        <kernelMemory>true</kernelMemory>
        </scanObjectSet>
        <scanBehaviour>
        <level>normal</level>
        <archives>{{scheduledScanArchives}}</archives>
        <pua>true</pua>
        <suspiciousFileDetection>false</suspiciousFileDetection>
        <scanForMacViruses>false</scanForMacViruses>
        <anti-rootkits>true</anti-rootkits>
        </scanBehaviour>
        <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
        </actions>
        <on-demand-options>
        <minimise-scan-impact>true</minimise-scan-impact>
        </on-demand-options>
        </settings>
        </scan>
        </scanSet>
  </onDemandScan>
</config>
)sophos";

    Task policyTask = { .taskType = Task::TaskType::Policy, .Content = invalidPolicyXml, .appId = "SAV" };
    m_taskQueue->push(policyTask);
    EXPECT_TRUE(waitForLog("Received SAV policy", 500ms));

    Task stopTask = { Task::TaskType::Stop, "" };
    m_taskQueue->push(stopTask);
    EXPECT_TRUE(waitForLog("Stop task received", 500ms));

    EXPECT_FALSE(appenderContains("SAV policy received for the first time."));
    EXPECT_FALSE(appenderContains("Processing request to restart sophos threat detector"));

    pluginThread.join();
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicy_ignoresPolicyWithWrongID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    expectedDefaultPolicyRequests(*mockBaseService);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink(_)).WillRepeatedly(::testing::Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, isFile(_)).WillOnce(Return(false));

    fs::path testDir = pluginInstall();
    const std::string expectedMd5 = "a1c0f318e58aad6bf90d07cabda54b7d"; // md5(md5("B:A"))
    const std::string customerIdFilePath1 = testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(testDir / "chroot") + customerIdFilePath1;
    Common::FileSystem::IFileSystemException ex(
        "Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(Plugin::getPersistThreatDatabaseFilePath(), "{}")).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).WillOnce(QueueStopTask(m_taskQueue));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generatePolicyXML(policy1revID, "99");
    std::string policy2Xml = generateUpdatePolicyXML(policy2revID);

    Task policy1Task ={ .taskType = Task::TaskType::Policy, .Content = policy1Xml, .appId = "SAV" };
    Task policy2Task = { .taskType = Task::TaskType::Policy, .Content = policy2Xml, .appId = "ALC" };
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy2Task);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received SAV policy"));
    EXPECT_TRUE(appenderContains("Ignoring unknown policy"));
    EXPECT_TRUE(appenderContains("Processing ALC policy: " + policy2Xml));
}

TEST_F(TestPluginAdapter, testProcessAction)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>)";
    Task actionTask = { Task::TaskType::Action, actionXml };
    m_taskQueue->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog));
    EXPECT_TRUE(waitForLog("Evaluating Scan Now", 500ms));

    m_taskQueue->pushStop();

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testProcessActionMalformed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="NONE" id="" subtype="MALFORMED" replyRequired="0"/>)";
    Task actionTask = { Task::TaskType::Action, actionXml };
    m_taskQueue->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog));
    EXPECT_FALSE(waitForLog("Evaluating Scan Now", 100ms));

    m_taskQueue->pushStop();

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testBadProcessActionXMLThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1">)";
    Task actionTask = { Task::TaskType::Action, actionXml };
    m_taskQueue->push(actionTask);

    EXPECT_TRUE(waitForLog("Exception encountered while parsing Action XML", 500ms));

    Task stopTask = { Task::TaskType::Stop, "" };
    m_taskQueue->push(stopTask);

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testProcessActionDoesntProcessJson)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"({someJson})";
    Task actionTask = { Task::TaskType::Action, actionXml };
    m_taskQueue->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog));
    EXPECT_TRUE(waitForLog("Ignoring action not in XML format"));

    m_taskQueue->pushStop();

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testProcessScanComplete)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);

    std::string scanCompleteXml = R"sophos(<?xml version="1.0"?>
        <event xmlns="http://www.sophos.com/EE/EESavEvent" type="sophos.mgt.sav.scanCompleteEvent">
          <defaultDescription>The scan has completed!</defaultDescription>
          <timestamp>20200101 120000</timestamp>
          <scanComplete>
            <scanName>Test Scan</scanName>
          </scanComplete>
          <entity></entity>
        </event>)sophos";

    EXPECT_CALL(*mockBaseService, sendEvent("SAV", scanCompleteXml));
    EXPECT_CALL(
        *mockBaseService, sendThreatHealth("{\"ThreatHealth\":" + std::to_string(E_THREAT_HEALTH_STATUS_GOOD) + "}"))
        .Times(0);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processScanComplete(scanCompleteXml);
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending scan complete notification to central: ";
    expectedLog.append(scanCompleteXml);

    EXPECT_TRUE(appenderContains(expectedLog));
}

TEST_F(TestPluginAdapter, testProcessThreatReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    std::string threatDetectedXML = "may be xml data";
    EXPECT_CALL(*mockBaseService, sendEvent("CORE", threatDetectedXML));

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    DetectionReporter::processThreatReport(threatDetectedXML, m_taskQueue);
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending threat detection notification to central: ";
    expectedLog.append(threatDetectedXML);

    EXPECT_TRUE(appenderContains(expectedLog));
}

TEST_F(TestPluginAdapter, publishQuarantineCleanEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    std::string threatDetectedXML = "may be xml data";
    EXPECT_CALL(*mockBaseService, sendEvent("CORE", threatDetectedXML));

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    DetectionReporter::publishQuarantineCleanEvent(threatDetectedXML, m_taskQueue);
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending Clean Event to Central: ";
    expectedLog.append(threatDetectedXML);

    EXPECT_TRUE(appenderContains(expectedLog));
}

TEST_F(TestPluginAdapter, processRestoreReport)
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);

    const RestoreReport restoreReport{ 0, "/tmp/eicar.txt", "correlationId", true };
    const auto eventXml = pluginimpl::generateCoreRestoreEventXml(restoreReport);
    EXPECT_CALL(*mockBaseService, sendEvent("CORE", eventXml));
    EXPECT_CALL(*mockBaseService, sendThreatHealth(R"({"ThreatHealth":1})")).Times(1);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

//    internal::CaptureStderr();
    {
        UsingMemoryAppender memoryAppenderHolder(*this);

        pluginAdapter.processRestoreReport(restoreReport);

        EXPECT_TRUE(waitForLog("Reporting successful restoration of /tmp/eicar.txt"));
        EXPECT_TRUE(waitForLog("Added restore report to task queue"));
    }
//    auto log = internal::GetCapturedStderr();
//    EXPECT_THAT(log, HasSubstr(" INFO Reporting successful restoration of /tmp/eicar.txt"));
//    EXPECT_THAT(log, HasSubstr("DEBUG Added restore report to task queue"));

    m_taskQueue->pushStop();

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
//        internal::CaptureStderr();
        pluginAdapter.mainLoop();
        EXPECT_TRUE(waitForLog("Sending Restore Event to Central: " + eventXml));
//        log = internal::GetCapturedStderr();
//        EXPECT_THAT(log, HasSubstr("DEBUG Sending Restore Event to Central: " + eventXml));
    }
}

class SubscriberThread
{
public:
    explicit SubscriberThread(Common::ZMQWrapperApi::IContext& context, const std::string& socketPath);
    ~SubscriberThread()
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }
    void start();

    std::vector<std::string> getData()
    {
        return m_data;
    };

private:
    Common::ZMQWrapperApi::IContext& m_context;
    Common::ZeroMQWrapper::ISocketSubscriberPtr m_subscriber;
    std::thread m_thread;
    std::vector<std::string> m_data;
    void run();
};

SubscriberThread::SubscriberThread(Common::ZMQWrapperApi::IContext& context, const std::string& socketPath) :
    m_context(context), m_subscriber(m_context.getSubscriber()), m_thread()
{
    m_subscriber->listen("ipc://" + socketPath);
    m_subscriber->subscribeTo("threatEvents");
}

void SubscriberThread::start()
{
    m_thread = std::thread(&SubscriberThread::run, this);
}

void SubscriberThread::run()
{
    try
    {
        m_data = m_subscriber->read();
        LOGINFO("Successfully read subscription data: " << m_data.at(0) << ", " << m_data.at(1));
    }
    catch (const Common::ZeroMQWrapper::IIPCException& e)
    {
        LOGERROR("Failed to read subscription data: " << e.what());
    }
}

TEST_F(TestPluginAdapter, testPublishThreatEvent)
{
    auto subscriberContext = Common::ZMQWrapperApi::createContext();
    SubscriberThread thread(*subscriberContext, m_threatEventPublisherSocketPath);
    thread.start();

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.connectToThreatPublishingSocket(m_threatEventPublisherSocketPath);

    std::string threatDetectedJSON = R"sophos({"threatName":"eicar", "threatPath":"/tmp/eicar.com"})sophos";
    // pluginAdapter appears to take a few moments to connect to the threatPublishingSocket, try a few times
    for (int tries = 0; tries < 10; ++tries)
    {
        pluginAdapter.publishThreatEvent(threatDetectedJSON);
        std::this_thread::sleep_for(10ms);
        if (!thread.getData().empty())
        {
            break;
        }
    }
    ASSERT_FALSE(thread.getData().empty());

    auto data = thread.getData();
    EXPECT_EQ(data.at(0), "threatEvents");
    EXPECT_EQ(data.at(1), threatDetectedJSON);
}

TEST_F(TestPluginAdapter, testPublishThreatHealth)
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string threatDetectedXML = R"sophos(
<notification>
  <threat name="Very bad file">
  </threat>
</notification>
            )sophos";
    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    DetectionReporter::processThreatReport(threatDetectedXML, m_taskQueue);
    m_taskQueue->pushStop();

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth(R"({"ThreatHealth":1})")).Times(1);
    pluginAdapter.publishThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth(R"({"ThreatHealth":2})")).Times(1);
    pluginAdapter.publishThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_SUSPICIOUS);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandScheduledScanThreatCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("Very bad file", E_SCAN_TYPE_SCHEDULED);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandScheduledScanThreatEicarCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("EICAR-AV-Test", E_SCAN_TYPE_SCHEDULED);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandMemoryScanThreatCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("Very bad file", E_SCAN_TYPE_MEMORY);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandMemoryScanThreatEicarCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("EICAR-AV-Test", E_SCAN_TYPE_MEMORY);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandThreatCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("Very bad file", E_SCAN_TYPE_ON_DEMAND);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandThreatEicarCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("EICAR-AV-Test", E_SCAN_TYPE_ON_DEMAND);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandUnknownThreatCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("Very bad file", E_SCAN_TYPE_UNKNOWN);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnDemandUnknownThreatEicarCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("EICAR-AV-Test", E_SCAN_TYPE_UNKNOWN);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-demand-threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnAccessThreatCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("Very bad file", E_SCAN_TYPE_ON_ACCESS_CLOSE);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-access-threat-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountOnAccessThreatEicarCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("EICAR-AV-Test", E_SCAN_TYPE_ON_ACCESS_OPEN);
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["on-access-threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapter, testPublishThreatHealthWithretrySuceedsAfterFailure)
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    Common::PluginApi::ApiException ex{ "dummy error" };
    EXPECT_CALL(*mockBaseService, sendThreatHealth(R"({"ThreatHealth":1})"))
        .WillOnce(Throw(ex))
        .WillOnce(Throw(ex))
        .WillOnce(Throw(ex))
        .WillOnce(Throw(ex))
        .WillOnce(Return());

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    m_taskQueue->pushStop();

    pluginAdapter.publishThreatHealthWithRetry(E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginAdapter, testPublishThreatHealthWithRetryFailsAfter5tries)
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    m_taskQueue->pushStop();
    Common::PluginApi::ApiException ex{ "dummy error" };
    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth(R"({"ThreatHealth":1})")).Times(5).WillRepeatedly(Throw(ex));

    pluginAdapter.publishThreatHealthWithRetry(E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginAdapter, testInvalidTaskType)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);

    PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    Task invalidTask = { static_cast<Task::TaskType>(99), "" };
    m_taskQueue->push(invalidTask);

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    Task policyTask = { .taskType = Task::TaskType::Policy, .Content = policyXml, .appId = "SAV" };
    m_taskQueue->push(policyTask);

    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received SAV policy"));
}

TEST_F(TestPluginAdapter, testCanStopWhileWaitingForFirstPolicies)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 1);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    m_taskQueue->pushStop();

    EXPECT_TRUE(waitForLog("Stopping the main program loop"));

    pluginThread.join();

    EXPECT_FALSE(appenderContains("ALC policy has not been sent to the plugin"));
    EXPECT_FALSE(appenderContains("SAV policy has not been sent to the plugin"));
}


TEST_F(TestPluginAdapter, testCanStopAfterReceivingFirstPolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string policyXml = R"sophos(<?xml version="1.0"?><invalidPolicy />)sophos";
    Task policyTask = { .taskType = Task::TaskType::Policy, .Content = policyXml, .appId = "SAV" };

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    expectedDefaultPolicyRequests(*mockBaseService);
    EXPECT_CALL(*mockBaseService, sendThreatHealth("{\"ThreatHealth\":1}")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus(_, _, _)).WillRepeatedly(Return());

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 1);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    m_taskQueue->push(policyTask);

    EXPECT_TRUE(waitForLog("Received SAV policy"));

    m_taskQueue->pushStop();

    EXPECT_TRUE(waitForLog("Stopping the main program loop"));

    pluginThread.join();

    EXPECT_FALSE(appenderContains("ALC policy has not been sent to the plugin"));
    EXPECT_FALSE(appenderContains("SAV policy has not been sent to the plugin"));
}