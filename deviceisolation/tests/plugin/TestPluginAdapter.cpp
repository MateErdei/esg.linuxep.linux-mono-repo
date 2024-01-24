// Copyright 2023-2024 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "MockNftWrapper.h"

#include "pluginimpl/NftWrapper.h"
#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/TaskQueue.h"
#include "pluginimpl/config.h"

#include "Common/XmlUtilities/AttributesMap.h"
#include "Common/PluginApi/NoPolicyAvailableException.h"
#include "Common/Helpers/MemoryAppender.h"
#include "Common/Helpers/MockApiBaseServices.h"
#include "Common/Helpers/MockProcess.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"

#include <gtest/gtest.h>
#include <future>
#include <memory>
#include <thread>

using namespace testing;
using namespace Plugin;

namespace
{

    constexpr auto* POLICY_EXAMPLE = R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <remoteAddress>12.12.12.12</remoteAddress>
                <localPort>80</localPort>
                <remotePort>22</remotePort>
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>)SOPHOS";

    constexpr auto* ENABLE_ACTION = R"SOPHOS(<?xml version="1.0" ?>
<action type="sophos.mgt.action.IsolationRequest">
     <enabled>true</enabled>
</action>)SOPHOS";

    class TestablePluginAdapter : public PluginAdapter
    {
    public:
        explicit TestablePluginAdapter(
                const std::shared_ptr<TaskQueue>& queueTask,
                std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseServiceApi,
                INftWrapperPtr nftWrapper
        ) :
                PluginAdapter(
                        queueTask,
                        std::move(baseServiceApi),
                        std::make_shared<PluginCallback>(queueTask), std::move(nftWrapper))
        {
        }

        explicit TestablePluginAdapter(
                const std::shared_ptr<TaskQueue>& queueTask,
                std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseServiceApi,
                std::shared_ptr<PluginCallback> callback,
                INftWrapperPtr nftWrapper
        ) :
                PluginAdapter(
                        queueTask,
                        std::move(baseServiceApi),
                        std::move(callback),
                        std::move(nftWrapper))
        {
        }

        void setQueueTimeout(std::chrono::milliseconds timeout)
        {
            queueTimeout_ = timeout;
        }

        void setWarnTimeout(std::chrono::milliseconds timeout)
        {
            warnMissingPolicyTimeout_ = timeout;
        }

        void setDebugTimeout(std::chrono::milliseconds timeout)
        {
            debugMissingPolicyTimeout_ = timeout;
        }
    };

    class PluginAdapterTests : public MemoryAppenderUsingTests
    {
    public:
        PluginAdapterTests() : MemoryAppenderUsingTests(PLUGIN_NAME)
        {}
    };
}

TEST_F(PluginAdapterTests, pluginRequestsNtpPolicyOnStart)
{
    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);
    queue->pushStop();
    plugin.mainLoop();
}

TEST_F(PluginAdapterTests, pluginGeneratesEmptyStatusWithoutPolicy)
{
    UsingMemoryAppender appender(*this);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);
    // Check logs
    EXPECT_TRUE(waitForLog("Failed to get NTP policy within ", std::chrono::milliseconds{20}));
    queue->pushStop();
    mainLoopFuture.get();

    // Check status - only sent once we do the warn
    ASSERT_NE(status, "");
    auto attrmap = Common::XmlUtilities::parseXml(status);
    auto elements = attrmap.lookupMultiple("status/csc:CompRes");
    ASSERT_EQ(elements.size(), 1);
    auto comp = elements.at(0);
    EXPECT_EQ(comp.value("RevID"), "");
    EXPECT_EQ(comp.value("Res"), "NoRef");
}

TEST_F(PluginAdapterTests, logsDebugForMissingPolicy)
{
    UsingMemoryAppender appender(*this);

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setDebugTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);
    EXPECT_TRUE(waitForLog("Timed out waiting for task"));
    EXPECT_TRUE(waitForLog("Failed to get NTP policy", std::chrono::milliseconds{20}));
    queue->pushStop();
    mainLoopFuture.get();

    EXPECT_TRUE(status.empty());
}

TEST_F(PluginAdapterTests, logsWarnForMissingPolicy)
{
    UsingMemoryAppender appender(*this);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);
    EXPECT_TRUE(waitForLog("Timed out waiting for task", std::chrono::milliseconds{20}));
    EXPECT_TRUE(waitForLog("Failed to get NTP policy", std::chrono::milliseconds{20}));
    queue->pushStop();
    mainLoopFuture.get();

    EXPECT_FALSE(status.empty());
}

TEST_F(PluginAdapterTests, pluginAdaptorContinuesIfPolicyNotAvailableImmediately)
{
    UsingMemoryAppender appender(*this);

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).WillOnce(Throw(Common::PluginApi::NoPolicyAvailableException()));
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setDebugTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);
    EXPECT_TRUE(waitForLog("NTP policy not available immediately"));
    queue->pushStop();
    mainLoopFuture.get();
}

TEST_F(PluginAdapterTests, pluginPutsRevidInStatus)
{
    constexpr const auto* REVID="ThisIsARevID";
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto queue = std::make_shared<TaskQueue>();
    queue->push(Task{
        .taskType = Task::TaskType::Policy,
        .Content = R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
</policy>
)SOPHOS",
        .appId = "NTP",
    });
    queue->pushStop();
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);
    plugin.mainLoop();

    ASSERT_NE(status, "");
    auto attrmap = Common::XmlUtilities::parseXml(status);
    auto elements = attrmap.lookupMultiple("status/csc:CompRes");
    ASSERT_EQ(elements.size(), 1);
    auto comp = elements.at(0);
    EXPECT_EQ(comp.value("RevID"), REVID);
    EXPECT_EQ(comp.value("Res"), "Same");
}


TEST_F(PluginAdapterTests, pluginHandlesInvalidPolicyXML)
{
    UsingMemoryAppender appender(*this);

    std::string policy{R"SOPHOS(<?xml version="1.0"?>
                        <policy>
                        <csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
                        <configuration>
                            <selfIsolation>
                            <exclusions>
                                <exclusion type=1234>
                                    <remotePort>22</remotePort>
                                </exclusion>
                            </exclusions>
                            </selfIsolation>
                        </configuration>
                        </policy>
                        )SOPHOS"};

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).Times(1);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    queue->push(Task{
            .taskType = Task::TaskType::Policy,
            .Content = policy,
            .appId = "NTP",
            });
    queue->pushStop();
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    EXPECT_NO_THROW(plugin.mainLoop());
    EXPECT_TRUE(waitForLog("NTPPolicyException encountered while parsing "));
}

TEST_F(PluginAdapterTests, pluginHandlesInvalidJsonPolicySilently)
{
    UsingMemoryAppender appender(*this);

    std::string jsonString = R"({"json": "contents"})";

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).Times(1);
    auto queue = std::make_shared<TaskQueue>();
    queue->push(Task{
            .taskType = Task::TaskType::Policy,
            .Content = jsonString,
            .appId = "NTP",
    });
    queue->pushStop();
    auto mockNftWrapper = std::make_shared<NaggyMock<MockNftWrapper>>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    EXPECT_NO_THROW(plugin.mainLoop());
    EXPECT_TRUE(waitForLog("Ignoring Json policy"));
}

class TestablePluginCallback : public PluginCallback
{
    public:
        TestablePluginCallback():
                PluginCallback(std::make_shared<TaskQueue>())
        {}
        void setIsolated(const bool isolate) override { isolated_ = isolate; }

        bool isolated_ = false;
};

TEST_F(PluginAdapterTests, nftWrapperReturnsSuccessEnableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS));

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    plugin.enableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Device is now isolated"));
    EXPECT_EQ(callback->isolated_, true);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsFailedEnableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::FAILED));

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    plugin.enableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Failed to isolate device"));
    EXPECT_EQ(callback->isolated_, false);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsRulesPresentNotIsolatedEnableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::RULES_ALREADY_PRESENT));

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    plugin.enableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Sophos rules are being enforced but isolation is not enabled yet"));
    EXPECT_TRUE(appenderContains("Tried to enable isolation but it was already enabled in the first place"));

    EXPECT_EQ(callback->isolated_, true);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsRulesPresentIsolatedEnableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::RULES_ALREADY_PRESENT));
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS)).RetiresOnSaturation();

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    //    Get into isolated state
    plugin.enableIsolation(mockNftWrapper);
    EXPECT_EQ(callback->isolated_, true);
    //    When in isolated state
    plugin.enableIsolation(mockNftWrapper);

    EXPECT_FALSE(appenderContains("Sophos rules are being enforced but isolation is not enabled yet"));
    EXPECT_TRUE(appenderContains("Tried to enable isolation but it was already enabled in the first place"));

    EXPECT_EQ(callback->isolated_, true);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsSuccessDisableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS));
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::SUCCESS));

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    //    Get into isolated state
    plugin.enableIsolation(mockNftWrapper);
    EXPECT_EQ(callback->isolated_, true);
    // Check result fo failure to exit
    plugin.disableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Device is no longer isolated"));
    EXPECT_EQ(callback->isolated_, false);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsFailedDisableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS));
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::FAILED));

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    //    Get into isolated state
    plugin.enableIsolation(mockNftWrapper);
    EXPECT_EQ(callback->isolated_, true);
    // Check result fo failure to exit
    plugin.disableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Failed to remove device from isolation"));
    EXPECT_EQ(callback->isolated_, true);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsRulesNotPresentNotIsolatedDisableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    plugin.disableIsolation(mockNftWrapper);

    EXPECT_FALSE(appenderContains("Failed to list sophos rules table, isolation is disabled"));
    EXPECT_TRUE(appenderContains("Tried to disable isolation but it was already disabled in the first place"));

    EXPECT_EQ(callback->isolated_, false);
}

TEST_F(PluginAdapterTests, nftWrapperReturnsRulesNotPresentIsolatedDisableIsolation)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS)).RetiresOnSaturation();

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();
    TestablePluginAdapter plugin(queue, mockBaseService, callback, mockNftWrapper);
    //    Get into isolated state
    plugin.enableIsolation(mockNftWrapper);
    EXPECT_EQ(callback->isolated_, true);
    //    When in isolated state
    plugin.disableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Failed to list sophos rules table, isolation is disabled"));
    EXPECT_EQ(callback->isolated_, false);
}

TEST_F(PluginAdapterTests, cannotEnableIsolationWithoutPolicy)
{
    UsingMemoryAppender appender(*this);
    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto queue = std::make_shared<TaskQueue>();

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);

    Task enableTask {.taskType = Plugin::Task::TaskType::Action, .Content = ENABLE_ACTION, .correlationId = "c123", .appId = "NTP"};
    queue->push(enableTask);
    EXPECT_TRUE(waitForLog("Ignoring enable Device Isolation action because there is no policy", std::chrono::milliseconds {50}));
    queue->pushStop();
    mainLoopFuture.get();
    EXPECT_EQ(plugin.isIsolationEnabled(), false);
}

TEST_F(PluginAdapterTests, canEnableIsolationWithPolicy)
{
    UsingMemoryAppender appender(*this);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));
    Plugin::IsolationExclusion exclusion;
    exclusion.setDirection(Plugin::IsolationExclusion::Direction::BOTH);
    exclusion.setLocalPorts({"80"});
    exclusion.setRemotePorts({"22"});
    exclusion.setRemoteAddressesAndIpTypes({{ "12.12.12.12", Plugin::IsolationExclusion::IPv4}});
    std::vector<Plugin::IsolationExclusion> allowListedIps = {exclusion};
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(allowListedIps)).WillOnce(Return(IsolateResult::SUCCESS));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);

    Task policyTask {.taskType = Plugin::Task::TaskType::Policy, .Content = POLICY_EXAMPLE, .appId = "NTP"};
    Task enableTask {.taskType = Plugin::Task::TaskType::Action, .Content = ENABLE_ACTION, .correlationId = "c123", .appId = "NTP"};
    queue->push(policyTask);
    queue->push(enableTask);
    EXPECT_TRUE(waitForLog("Enabling Device Isolation", std::chrono::milliseconds {50}));
    queue->pushStop();
    mainLoopFuture.get();
    EXPECT_EQ(plugin.isIsolationEnabled(), true);
}

TEST_F(PluginAdapterTests, pluginPersistsIsolatedModeToDisk)
{
    UsingMemoryAppender appender(*this);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    // Make sure we read the persistent values on start up
    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationEnabled")).WillOnce(
            Return(false));
    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationActionValue")).WillOnce(
            Return(false));

    // Write the enabled persistent value to disk twice - once when we enable and once when we shutdown.
    EXPECT_CALL(*mockFileSystem, writeFile("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationEnabled", "1")).Times(2);

    // Write the central action persistent value to disk twice - once when we get the action and once when we shutdown.
    EXPECT_CALL(*mockFileSystem, writeFile("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationActionValue", "1")).Times(2);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockBaseService = std::make_shared<NaggyMock<MockApiBaseServices>>();

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);

    Task policyTask {.taskType = Plugin::Task::TaskType::Policy, .Content = POLICY_EXAMPLE, .appId = "NTP"};
    Task enableTask {.taskType = Plugin::Task::TaskType::Action, .Content = ENABLE_ACTION, .correlationId = "c123", .appId = "NTP"};
    queue->push(policyTask);
    EXPECT_TRUE(waitForLog("Device Isolation policy applied", std::chrono::milliseconds {50}));
    queue->push(enableTask);
    EXPECT_TRUE(waitForLog("Enabling Device Isolation", std::chrono::milliseconds {50}));
    queue->pushStop();
    mainLoopFuture.get();
    EXPECT_EQ(plugin.isIsolationEnabled(), true);
}

TEST_F(PluginAdapterTests, pluginStartsUpIsolatedIfItWasEnabledBeforeStopping)
{
    UsingMemoryAppender appender(*this);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    // Make sure we read the persistent values on start up
    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationEnabled")).WillOnce(
            Return(true));
    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationActionValue")).WillOnce(
            Return(true));

    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationEnabled")).Times(1).WillOnce(
            Return("1"));
    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/plugins/deviceisolation/var/persist-isolationActionValue")).Times(1).WillOnce(
            Return("1"));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockBaseService = std::make_shared<NaggyMock<MockApiBaseServices>>();

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    // To apply policy rules we clear them first, so an enable triggered by policy change will always clear first
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).Times(1);
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    Task policyTask {.taskType = Plugin::Task::TaskType::Policy, .Content = POLICY_EXAMPLE, .appId = "NTP"};
    queue->push(policyTask);
    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);
    EXPECT_TRUE(waitForLog("Device Isolation policy applied", std::chrono::milliseconds {50}));
    EXPECT_TRUE(waitForLog("Enabling Device Isolation", std::chrono::milliseconds {50}));
    EXPECT_EQ(plugin.isIsolationEnabled(), true);
    queue->pushStop();
    mainLoopFuture.get();
    EXPECT_EQ(plugin.isIsolationEnabled(), true);
}

TEST_F(PluginAdapterTests, isolationIsEnabledOncePolicyIsReceivedIfDeviceWasIsolatedBeforeGettingPolicy)
{
    UsingMemoryAppender appender(*this);

    auto mockFileSystem = std::make_unique<NaggyMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::RULES_NOT_PRESENT));
    Plugin::IsolationExclusion exclusion;
    exclusion.setDirection(Plugin::IsolationExclusion::Direction::BOTH);
    exclusion.setLocalPorts({"80"});
    exclusion.setRemotePorts({"22"});
    exclusion.setRemoteAddressesAndIpTypes({{ "12.12.12.12", Plugin::IsolationExclusion::IPv4}});
    std::vector<Plugin::IsolationExclusion> allowListedIps = {exclusion};
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(allowListedIps)).WillOnce(Return(IsolateResult::SUCCESS));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService, mockNftWrapper);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setWarnTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);

    Task policyTask {.taskType = Plugin::Task::TaskType::Policy, .Content = POLICY_EXAMPLE, .appId = "NTP"};
    Task enableTask {.taskType = Plugin::Task::TaskType::Action, .Content = ENABLE_ACTION, .correlationId = "c123", .appId = "NTP"};
    EXPECT_EQ(plugin.isIsolationEnabled(), false);
    queue->push(enableTask);
    EXPECT_EQ(plugin.isIsolationEnabled(), false);
    EXPECT_TRUE(waitForLog("Ignoring enable Device Isolation action because there is no policy", std::chrono::milliseconds {50}));
    queue->push(policyTask);
    EXPECT_TRUE(waitForLog("Retrying isolation, triggered by policy being received", std::chrono::milliseconds {50}));
    EXPECT_TRUE(waitForLog("Enabling Device Isolation", std::chrono::milliseconds {50}));
    queue->pushStop();
    mainLoopFuture.get();
    EXPECT_EQ(plugin.isIsolationEnabled(), true);
}