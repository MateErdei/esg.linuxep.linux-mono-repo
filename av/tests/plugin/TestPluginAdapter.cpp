/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginMemoryAppenderUsingTests.h"
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockApiBaseServices.h>

#include <pluginimpl/PluginAdapter.h>
#include <pluginimpl/PluginAdapter.cpp>
#include "datatypes/sophos_filesystem.h"
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tests/common/Common.h>

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

            m_queueTask = std::make_shared<QueueTask>();
            m_callback = std::make_shared<Plugin::PluginCallback>(m_queueTask);
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

        std::shared_ptr<QueueTask> m_queueTask;
        std::shared_ptr<Plugin::PluginCallback> m_callback;
        fs::path m_threatEventPublisherSocketPath;
    };
}

ACTION_P(QueueStopTask, taskQueue) {
    taskQueue->pushStop();
}

TEST_F(TestPluginAdapter, testConstruction) //NOLINT
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
}


TEST_F(TestPluginAdapter, testMainLoop) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillOnce(QueueStopTask(m_queueTask));
    pluginAdapter.mainLoop();
}


TEST_F(TestPluginAdapter, testRequestPoliciesThrows) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    Common::PluginApi::ApiException ex { "dummy error" };
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).WillOnce(Throw(ex));
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).WillOnce(Throw(ex));

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_queueTask->push(policyTask);

    m_queueTask->pushStop();
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Failed to request SAV policy at startup (dummy error)"));
    EXPECT_TRUE(appenderContains("Failed to request ALC policy at startup (dummy error)"));
}

TEST_F(TestPluginAdapter, testProcessPolicy) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();

    fs::path testDir = tmpdir();
    const std::string susiStartupSettingsPath = testDir / "var/susi_startup_settings.json";
    const std::string susiStartupSettingsChrootPath = std::string(testDir / "chroot") + susiStartupSettingsPath;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + susiStartupSettingsPath + "', file does not exist");

    EXPECT_CALL(*mockIFileSystemPtr, readFile(_)).WillRepeatedly(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(_,_)).WillRepeatedly(Throw(ex));
    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(
        m_queueTask,
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
    m_queueTask->push(policy1Task);
    m_queueTask->push(policy2Task);
    m_queueTask->push(policy3Task);

    std::string initialStatusXml = generateStatusXML("NoRef", "");
    std::string status1Xml = generateStatusXML("Same", policy1revID);
    std::string status2Xml = generateStatusXML("Same", policy2revID);
    std::string status3Xml = generateStatusXML("Same", policy3revID);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status1Xml, status1Xml)).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status3Xml, status3Xml)).WillOnce(QueueStopTask(m_queueTask));
    EXPECT_EQ(m_callback->getStatus("SAV").statusXml, initialStatusXml);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy1Xml));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy2Xml));
    EXPECT_TRUE(appenderContains("Received new policy with revision ID: 123"));
    // We now see all of the restart events
    EXPECT_EQ(appenderCount("Processing request to restart sophos threat detector"), 3);
    EXPECT_EQ(appenderCount("Requesting scan monitor to reload susi"), 1);
}

TEST_F(TestPluginAdapter, testWaitForTheFirstPolicyReturnsEmptyPolicyOnInvalidPolicy) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";

    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_queueTask->push(policyTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(
        m_queueTask,
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

    m_queueTask->pushStop();

    pluginThread.join();

}

TEST_F(TestPluginAdapter, testProcessPolicy_ignoresPolicyWithWrongID) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generatePolicyXML(policy1revID, "1");
    std::string policy2Xml = generatePolicyXML(policy2revID);

    Task policy1Task = {Task::TaskType::Policy, policy1Xml};
    Task policy2Task = {Task::TaskType::Policy, policy2Xml};
    m_queueTask->push(policy1Task);
    m_queueTask->push(policy2Task);

    std::string initialStatusXml = generateStatusXML("NoRef", "");
    std::string status1Xml = generateStatusXML("Same", policy1revID);
    std::string status2Xml = generateStatusXML("Same", policy2revID);
    EXPECT_CALL(*mockBaseServicePtr, sendStatus("SAV", status2Xml, status2Xml)).WillOnce(QueueStopTask(m_queueTask));

    EXPECT_EQ(m_callback->getStatus("SAV").statusXml, initialStatusXml);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy2Xml));
    EXPECT_TRUE(appenderContains("Ignoring policy of incorrect type: 1"));
    EXPECT_TRUE(appenderContains("Received new policy with revision ID: 123"));
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicy) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();

    fs::path testDir = tmpdir();
    const std::string expectedMd5 = "a1c0f318e58aad6bf90d07cabda54b7d"; // md5(md5("B:A"))
    const std::string customerIdFilePath1 = testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(testDir / "chroot") + customerIdFilePath1;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).WillOnce(QueueStopTask(m_queueTask));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policyRevID = "12345678901";
    std::string policyXml = generateUpdatePolicyXML(policyRevID);

    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_queueTask->push(policyTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policyXml));
}

TEST_F(TestPluginAdapter, testProcessUpdatePolicy_ignoresPolicyWithWrongID) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();

    fs::path testDir = tmpdir();
    const std::string expectedMd5 = "a1c0f318e58aad6bf90d07cabda54b7d"; // md5(md5("B:A"))
    const std::string customerIdFilePath1 = testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(testDir / "chroot") + customerIdFilePath1;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).WillOnce(QueueStopTask(m_queueTask));

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    std::string policy1revID = "12345678901";
    std::string policy2revID = "12345678902";
    std::string policy1Xml = generateUpdatePolicyXML(policy1revID, "2");
    std::string policy2Xml = generateUpdatePolicyXML(policy2revID);

    Task policy1Task = {Task::TaskType::Policy, policy1Xml};
    Task policy2Task = {Task::TaskType::Policy, policy2Xml};
    m_queueTask->push(policy1Task);
    m_queueTask->push(policy2Task);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
    EXPECT_TRUE(appenderContains("Ignoring policy of incorrect type: 2"));
    EXPECT_TRUE(appenderContains("Processing policy: " + policy2Xml));
}

TEST_F(TestPluginAdapter, testProcessAction) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);


    auto pluginAdapter = std::make_shared<PluginAdapter>(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop", 500ms));
    EXPECT_TRUE(waitForLog("Starting scanScheduler", 500ms));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>)";
    Task actionTask = {Task::TaskType::Action, actionXml};
    m_queueTask->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog, 500ms));
    EXPECT_TRUE(waitForLog("Evaluating Scan Now", 500ms));

    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testProcessActionMalformed) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger scanLogger = Common::Logging::getInstance("ScanScheduler");
    scanLogger.addAppender(m_sharedAppender);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop", 500ms));
    EXPECT_TRUE(waitForLog("Starting scanScheduler", 500ms));

    std::string actionXml =
        R"(<?xml version='1.0'?><a:action xmlns:a="com.sophos/msys/action" type="NONE" id="" subtype="MALFORMED" replyRequired="0"/>)";
    Task actionTask = {Task::TaskType::Action, actionXml};
    m_queueTask->push(actionTask);

    std::string expectedLog = "Process action: ";
    expectedLog.append(actionXml);

    EXPECT_TRUE(waitForLog(expectedLog, 500ms));
    EXPECT_FALSE(waitForLog("Evaluating Scan Now", 500ms));

    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    pluginThread.join();

    scanLogger.removeAppender(m_sharedAppender);
}

TEST_F(TestPluginAdapter, testProcessScanComplete) //NOLINT
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
    EXPECT_CALL(*mockBaseServicePtr, sendThreatHealth("{\"ThreatHealth\":" + std::to_string(E_THREAT_HEALTH_STATUS_GOOD) + "}")).Times(1);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processScanComplete(scanCompleteXml, common::E_CLEAN_SUCCESS);
    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();
    std::string expectedLog = "Sending scan complete notification to central: ";
    expectedLog.append(scanCompleteXml);

    EXPECT_TRUE(appenderContains(expectedLog));

}

TEST_F(TestPluginAdapter, testProcessThreatReport) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
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

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processThreatReport(threatDetectedXML);
    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
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

TEST_F(TestPluginAdapter, testPublishThreatReport) //NOLINT
{
    auto subscriberContext = Common::ZMQWrapperApi::createContext();
    SubscriberThread thread(*subscriberContext, m_threatEventPublisherSocketPath);
    thread.start();

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.connectToThreatPublishingSocket(m_threatEventPublisherSocketPath);
    std::string threatDetectedJSON = R"sophos({"threatName":"eicar", "threatPath":"/tmp/eicar.com"})sophos";

    while (thread.getData().empty())
    {
        pluginAdapter.publishThreatEvent(threatDetectedJSON);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto data = thread.getData();
    EXPECT_EQ(data.at(0), "threatEvents");
    EXPECT_EQ(data.at(1), threatDetectedJSON);
}

TEST_F(TestPluginAdapter, testProcessThreatReportIncrementsThreatCount) //NOLINT
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

    EXPECT_CALL(*mockBaseServicePtr, sendEvent("SAV", threatDetectedXML));

    Common::Telemetry::TelemetryHelper::getInstance().reset();

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processThreatReport(threatDetectedXML);
    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();

    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    std::string ExpectedTelemetry{ R"sophos({"threat-count":1})sophos" };
    EXPECT_EQ(telemetryResult, ExpectedTelemetry);
}

TEST_F(TestPluginAdapter, testProcessThreatReportIncrementsThreatEicarCount) //NOLINT
{
    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices> >();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    std::string threatDetectedXML = R"sophos(
<notification>
  <threat name="EICAR-AV-Test">
  </threat>
</notification>
            )sophos";

    EXPECT_CALL(*mockBaseServicePtr, sendEvent("SAV", threatDetectedXML));

    Common::Telemetry::TelemetryHelper::getInstance().reset();

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);
    pluginAdapter.processThreatReport(threatDetectedXML);
    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);
    pluginAdapter.mainLoop();

    auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
    std::string ExpectedTelemetry{ R"sophos({"threat-eicar-count":1})sophos" };
    EXPECT_EQ(telemetryResult, ExpectedTelemetry);
}

TEST_F(TestPluginAdapter, testInvalidTaskType) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    PluginAdapter pluginAdapter(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 0);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);

    Task invalidTask = {static_cast<Task::TaskType>(99), ""};
    m_queueTask->push(invalidTask);

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<invalidPolicy />
)sophos";
    Task policyTask = {Task::TaskType::Policy, policyXml};
    m_queueTask->push(policyTask);

    Task stopTask = {Task::TaskType::Stop, ""};
    m_queueTask->push(stopTask);

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Received Policy"));
}


TEST_F(TestPluginAdapter, testCanStopWhileWaitingForFirstPolicies) //NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockBaseService = std::make_unique<StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("SAV")).Times(1);
    EXPECT_CALL(*mockBaseServicePtr, requestPolicies("ALC")).Times(1);

    auto pluginAdapter = std::make_shared<PluginAdapter>(m_queueTask, std::move(mockBaseService), m_callback, m_threatEventPublisherSocketPath, 1);
    auto pluginThread = std::thread(&PluginAdapter::mainLoop, pluginAdapter);

    EXPECT_TRUE(waitForLog("Starting the main program loop", 500ms));

    m_queueTask->pushStop();

    EXPECT_TRUE(waitForLog("Stopping the main program loop", 500ms));

    pluginThread.join();

    EXPECT_FALSE(appenderContains("ALC policy has not been sent to the plugin"));
    EXPECT_FALSE(appenderContains("SAV policy has not been sent to the plugin"));
}