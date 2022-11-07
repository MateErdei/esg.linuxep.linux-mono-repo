// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define PLUGIN_INTERNAL public

#include "PluginMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/Logger.h"
#include "pluginimpl/PluginAdapter.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockApiBaseServices.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/common/Common.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <future>

using namespace testing;
using namespace Plugin;

namespace fs = sophos_filesystem;

namespace
{
    class TestPluginAdapter : public PluginMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            fs::path basePath = fs::temp_directory_path();
            basePath /= test_info->test_case_name();
            basePath /= test_info->name();
            fs::remove_all(basePath);
            fs::create_directories(basePath);
            m_threatEventPublisherSocketPath = basePath / "threatEventPublisherSocket";

            setupFakeSophosThreatDetectorConfig();

            m_taskQueue = std::make_shared<TaskQueue>();
            m_callback = std::make_shared<Plugin::PluginCallback>(m_taskQueue);
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        static std::string generatePolicyXML(const std::string& revID, const std::string& policyID="2", const std::string& sxl="false")
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
)sophos", {{"@@REV_ID@@", revID},
           {"@@POLICY_ID@@", policyID},
           {"@@SXL@@", sxl}});
        }

        static std::string generateUpdatePolicyXML(const std::string& revID, const std::string& policyID="1")
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
)sophos", {{"@@REV_ID@@", revID},
           {"@@POLICY_ID@@", policyID}});
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
</status>)sophos", {
                        {"@@POLICY_COMPLIANCE@@", res},
                        {"@@REV_ID@@", revID}
                    });
        }

        std::shared_ptr<TaskQueue> m_taskQueue;
        std::shared_ptr<Plugin::PluginCallback> m_callback;
        fs::path m_threatEventPublisherSocketPath;
    };
}

ACTION_P(QueueStopTask, taskQueue) {
    taskQueue->pushStop();
}

TEST_F(TestPluginAdapter, testConstruction)
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
}


TEST_F(TestPluginAdapter, testMainLoop)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillOnce(QueueStopTask(m_taskQueue));
    pluginAdapter.mainLoop();
}


TEST_F(TestPluginAdapter, testRequestPoliciesThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    Common::PluginApi::ApiException ex { "dummy error" };
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).WillOnce(Throw(ex));
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillOnce(Throw(ex));
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).WillOnce(Throw(ex));

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_taskQueue->push(policyTask);

    m_taskQueue->pushStop();
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
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

    fs::path testDir = tmpdir();
    const std::string susiStartupSettingsPath = testDir / "var/susi_startup_settings.json";
    const std::string susiStartupSettingsChrootPath = std::string(testDir / "chroot") + susiStartupSettingsPath;
    const std::string databasePath = std::string(testDir / "var/persist-threatDatabase") ;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + susiStartupSettingsPath + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, exists(databasePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, readFile(_)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(_,_)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFileAtomically(_,_,_,_)).WillRepeatedly(Throw(ex));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_taskQueue,
        std::move(mockBaseService),
        m_callback,
        m_threatEventPublisherSocketPath,
        0
        );

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy3revID = "12345678903";
    std::string policy1Xml = generatePolicyXML(policy1revID);
    std::string policy2Xml = generatePolicyXML(policy2revID);
    std::string policy3Xml = generatePolicyXML(policy3revID, "2", "true");

    Task policy1Task = {Task::TaskType::Policy, policy1Xml};
    Task policy2Task = {Task::TaskType::Policy, policy2Xml};
    Task policy3Task = {Task::TaskType::Policy, policy3Xml};
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

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy1Xml));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy2Xml));
    EXPECT_TRUE(appenderContains("Received new policy with revision ID: 123"));
    // We now see all of the restart events
    EXPECT_EQ(appenderCount("Processing request to restart sophos threat detector"), 3);
    EXPECT_EQ(appenderCount("Requesting scan monitor to reload susi"), 1);
}

TEST_F(TestPluginAdapter, testWaitForTheFirstPolicyReturnsEmptyPolicyOnInvalidPolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";

    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_taskQueue->push(policyTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_taskQueue,
        std::move(mockBaseService),
        m_callback,
        m_threatEventPublisherSocketPath,
        1
        );
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    std::this_thread::sleep_for(500ms);

    EXPECT_FALSE(appenderContains("ALC policy has not been sent to the plugin"));
    EXPECT_FALSE(appenderContains("SAV policy has not been sent to the plugin"));

    EXPECT_TRUE(waitForLog("ALC policy has not been sent to the plugin", 700ms));
    EXPECT_TRUE(waitForLog("SAV policy has not been sent to the plugin", 200ms));

    m_taskQueue->pushStop();

    pluginThread.join();

}

TEST_F(TestPluginAdapter, testProcessPolicy_ignoresPolicyWithWrongID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue,  std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generatePolicyXML(policy1revID, "1");
    std::string policy2Xml = generatePolicyXML(policy2revID);

    Task policy1Task = {Task::TaskType::Policy, policy1Xml};
    Task policy2Task = {Task::TaskType::Policy, policy2Xml};
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy2Task);

    std::string initialStatusXml = generateStatusXML("NoRef", "");
    std::string status1Xml = generateStatusXML("Same", policy1revID);
    std::string status2Xml = generateStatusXML("Same", policy2revID);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).WillOnce(QueueStopTask(m_taskQueue));

    EXPECT_EQ(m_callback->getStatus("SAV").statusXml, initialStatusXml);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy2Xml));
    EXPECT_TRUE(appenderContains("Ignoring policy of incorrect type: 1"));
    EXPECT_TRUE(appenderContains("Received new policy with revision ID: 123"));
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink(_)).WillRepeatedly(::testing::Return(std::nullopt));

    fs::path testDir = tmpdir();
    const std::string expectedMd5 = "a1c0f318e58aad6bf90d07cabda54b7d"; // md5(md5("B:A"))
    const std::string customerIdFilePath1 = testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(testDir / "chroot") + customerIdFilePath1;
    const std::string databasePath = std::string(testDir / "var/persist-threatDatabase") ;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, exists(databasePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(databasePath, "{}")).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).WillOnce(QueueStopTask(m_taskQueue));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policyRevID = "12345678901";
    std::string policyXml = generateUpdatePolicyXML(policyRevID);

    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_taskQueue->push(policyTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policyXml));
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicyThrowsIfInvalidXML)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger threadRunnerLogger = Common::Logging::getInstance("Common");
    threadRunnerLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
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
        )sophos", {{"@@REV_ID@@", "12345678901"},
                    {"@@POLICY_ID@@", "2"},
                    {"@@SXL@@", "false"}});

    Task policyTask = {Task::TaskType::Policy, brokenPolicyXml};
    m_taskQueue->push(policyTask);

    EXPECT_TRUE(waitForLog("Exception encountered while parsing AV policy XML: Error parsing xml"));

    Task stopTask = {Task::TaskType::Stop, ""};
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
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
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

    Task policyTask = {Task::TaskType::Policy, invalidPolicyXml};
    m_taskQueue->push(policyTask);
    EXPECT_TRUE(waitForLog("Received Policy"));

    Task stopTask = {Task::TaskType::Stop, ""};
    m_taskQueue->push(stopTask);
    EXPECT_TRUE(waitForLog("Stop task received"));

    EXPECT_FALSE(appenderContains("SAV policy received for the first time."));
    EXPECT_FALSE(appenderContains("Processing request to restart sophos threat detector"));

    pluginThread.join();
}


TEST_F(TestPluginAdapter, testProcessUpdatePolicy_ignoresPolicyWithWrongID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink(_)).WillRepeatedly(::testing::Return(std::nullopt));

    fs::path testDir = tmpdir();
    const std::string expectedMd5 = "a1c0f318e58aad6bf90d07cabda54b7d"; // md5(md5("B:A"))
    const std::string customerIdFilePath1 = testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(testDir / "chroot") + customerIdFilePath1;
    const std::string databasePath = std::string(testDir / "var/persist-threatDatabase") ;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, exists(databasePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(databasePath, "{}")).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).WillOnce(QueueStopTask(m_taskQueue));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generateUpdatePolicyXML(policy1revID, "2");
    std::string policy2Xml = generateUpdatePolicyXML(policy2revID);

    Task policy1Task = {Task::TaskType::Policy, policy1Xml};
    Task policy2Task = {Task::TaskType::Policy, policy2Xml};
    m_taskQueue->push(policy1Task);
    m_taskQueue->push(policy2Task);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Ignoring policy of incorrect type: 2"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy2Xml));
}

TEST_F(TestPluginAdapter, testProcessAction)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);


    auto pluginAdapter = std::make_shared<PluginAdapter>(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>)";
    Task actionTask = {Task::TaskType::Action, actionXml};
    m_taskQueue->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog));
    EXPECT_TRUE(waitForLog("Evaluating Scan Now"));

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
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="NONE" id="" subtype="MALFORMED" replyRequired="0"/>)";
    Task actionTask = {Task::TaskType::Action, actionXml};
    m_taskQueue->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog));
    EXPECT_FALSE(waitForLog("Evaluating Scan Now"));

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
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1">)";
    Task actionTask = {Task::TaskType::Action, actionXml};
    m_taskQueue->push(actionTask);

    EXPECT_TRUE(waitForLog("Exception encountered while parsing Action XML"));

    Task stopTask = {Task::TaskType::Stop, ""};
    m_taskQueue->push(stopTask);

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testProcessScanComplete)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string scanCompleteXml = R"sophos(<?xml version="1.0"?>
        <event xmlns="http://www.sophos.com/EE/EESavEvent" type="sophos.mgt.sav.scanCompleteEvent">
          <defaultDescription>The scan has completed!</defaultDescription>
          <timestamp>20200101 120000</timestamp>
          <scanComplete>
            <scanName>Test Scan</scanName>
          </scanComplete>
          <entity></entity>
        </event>)sophos";

    EXPECT_CALL(*mockBaseServicePtr, sendEvent("SAV", scanCompleteXml));
    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":" + std::to_string(E_THREAT_HEALTH_STATUS_GOOD) + "}")).Times(0);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processScanComplete(scanCompleteXml);
    m_taskQueue->pushStop();

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending scan complete notification to central: ";
    expectedLog.append(scanCompleteXml);

    EXPECT_TRUE(appenderContains(expectedLog));

}

TEST_F(TestPluginAdapter, testProcessThreatReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string threatDetectedXML = "may be xml data";

    EXPECT_CALL(*mockBaseServicePtr, sendEvent("CORE", threatDetectedXML));

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processThreatReport(threatDetectedXML);
    m_taskQueue->pushStop();

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending threat detection notification to central: ";
    expectedLog.append(threatDetectedXML);

    EXPECT_TRUE(appenderContains(expectedLog));
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
    m_context(context),
    m_subscriber(m_context.getSubscriber()),
    m_thread()
{
    m_subscriber->listen("ipc://" + socketPath);
    m_subscriber->subscribeTo("threatEvents");
}

void SubscriberThread::start() { m_thread = std::thread(&SubscriberThread::run, this); }

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

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.connectToThreatPublishingSocket(m_threatEventPublisherSocketPath);

    std::string threatDetectedJSON = R"sophos({"threatName":"eicar", "threatPath":"/tmp/eicar.com"})sophos";
    // pluginAdapter appears to take a few moments to connect to the threatPublishingSocket, try a few times
    for (int tries=0; tries<10; ++tries)
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
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth(R"({"ThreatHealth":1})")).Times(1);
    pluginAdapter.publishThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_GOOD);

    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth(R"({"ThreatHealth":2})")).Times(1);
    pluginAdapter.publishThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
    EXPECT_EQ(m_callback->getThreatHealth(), E_THREAT_HEALTH_STATUS_SUSPICIOUS);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountIncrementsThreatCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("Very bad file");
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["threat-count"], 1);
}

TEST_F(TestPluginAdapter, testIncrementTelemetryThreatCountIncrementsThreatEicarCount)
{
    Common::Telemetry::TelemetryHelper::getInstance().reset();
    PluginAdapter::incrementTelemetryThreatCount("EICAR-AV-Test");
    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    auto telemetry = nlohmann::json::parse(telemetryResult);
    EXPECT_EQ(telemetry["threat-eicar-count"], 1);
}

TEST_F(TestPluginAdapter, testInvalidTaskType)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    Task invalidTask = {static_cast<Task::TaskType>(99), ""};
    m_taskQueue->push(invalidTask);

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_taskQueue->push(policyTask);

    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
}


TEST_F(TestPluginAdapter, testCanStopWhileWaitingForFirstPolicies)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("FLAGS")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 1);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop"));

    m_taskQueue->pushStop();

    EXPECT_TRUE(waitForLog("Stopping the main program loop"));

    pluginThread.join();

    EXPECT_FALSE(appenderContains("ALC policy has not been sent to the plugin"));
    EXPECT_FALSE(appenderContains("SAV policy has not been sent to the plugin"));
}


// TODO: LINUXDAR-5806 -- stablise this test
//// PluginAdapter construction needed for this test, so it's here instead of its own TestSafeStoreWorker file (for now)
//TEST_F(TestPluginAdapter, testSafeStoreWorkerExitsOnStop) // NOLINT
//{
//    // In lieu of a Mock PluginAdapter, make a real one since this test never calls its functions anyway
//    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
//    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
//    ASSERT_NE(mockBaseServicePtr, nullptr);
//    PluginAdapter pluginAdapter(
//        m_taskQueue, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
//
//    auto newQueue = std::make_shared<DetectionQueue>();
//    auto worker = std::make_shared<SafeStoreWorker>(pluginAdapter, newQueue, "fakeSocket");
//
//    // Prove worker exits when queue gets stop request
//    auto result = std::async(std::launch::async, &SafeStoreWorker::join, worker);
//    worker->start();
//    newQueue->requestStop();
//    ASSERT_EQ(result.wait_for(std::chrono::milliseconds(50)), std::future_status::ready);
//    result.get();
//
//    // Prove worker exits when thread is asked to stop, detection given to queue to unblock internal pop
//    result = std::async(std::launch::async, &SafeStoreWorker::join, worker);
//    worker->start();
//    worker->requestStop();
//    scan_messages::ThreatDetected basicDetection(
//        "root",
//        1,
//        scan_messages::E_VIRUS_THREAT_TYPE,
//        "threatName",
//        scan_messages::E_SCAN_TYPE_UNKNOWN,
//        scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
//        "/path",
//        scan_messages::E_SMT_THREAT_ACTION_UNKNOWN,
//        "sha256",
//        "threatId",
//        datatypes::AutoFd());
//    newQueue->push(basicDetection);
//    ASSERT_EQ(result.wait_for(std::chrono::milliseconds(50)), std::future_status::ready);
//    result.get();
//}