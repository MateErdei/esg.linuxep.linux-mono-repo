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
    class TestablePluginAdapter : public PluginAdapter
    {
    public:
        explicit TestablePluginAdapter(
                const std::shared_ptr<TaskQueue>& queueTask,
                std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseServiceApi
        ) :
                PluginAdapter(
                        queueTask,
                        std::move(baseServiceApi),
                        std::make_shared<PluginCallback>(queueTask))
        {
        }

        explicit TestablePluginAdapter(
                const std::shared_ptr<TaskQueue>& queueTask,
                std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseServiceApi,
                std::shared_ptr<PluginCallback> callback
        ) :
                PluginAdapter(
                        queueTask,
                        std::move(baseServiceApi),
                        std::move(callback))
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
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService);
    queue->pushStop();
    plugin.mainLoop();
}

TEST_F(PluginAdapterTests, pluginGeneratesEmptyStatusWithoutPolicy)
{
    UsingMemoryAppender appender(*this);

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService);

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
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService);

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

    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService);

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

    auto queue = std::make_shared<TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService);

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
    TestablePluginAdapter plugin(queue, mockBaseService);
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
    auto queue = std::make_shared<TaskQueue>();
    queue->push(Task{
            .taskType = Task::TaskType::Policy,
            .Content = policy,
            .appId = "NTP",
            });
    queue->pushStop();

    TestablePluginAdapter plugin(queue, mockBaseService);

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

    TestablePluginAdapter plugin(queue, mockBaseService);

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

TEST_F(PluginAdapterTests, pluginEnablesIsolationCorrectly)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, applyIsolateRules(_)).WillOnce(Return(IsolateResult::SUCCESS));

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, writeFile(_, "1")).Times(2);
    //Persist value when destroyed and when we set it on enabling isolation
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();

    TestablePluginAdapter plugin(queue, mockBaseService, callback);
    plugin.enableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Device is now isolated"));
    EXPECT_EQ(callback->isolated_, true);
}

TEST_F(PluginAdapterTests, pluginDisablesIsolationCorrectly)
{
    UsingMemoryAppender appender(*this);

    auto mockNftWrapper = std::make_shared<StrictMock<MockNftWrapper>>();
    EXPECT_CALL(*mockNftWrapper, clearIsolateRules()).WillOnce(Return(IsolateResult::SUCCESS));

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    //Persist value when destroyed and when we set it on disabling isolation
    EXPECT_CALL(*mockFileSystem, writeFile(_, "0")).Times(2);
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto queue = std::make_shared<TaskQueue>();
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    auto callback = std::make_shared<TestablePluginCallback>();

    TestablePluginAdapter plugin(queue, mockBaseService, callback);
    plugin.disableIsolation(mockNftWrapper);

    EXPECT_TRUE(appenderContains("Device is no longer isolated"));
    EXPECT_EQ(callback->isolated_, false);
}

//Todo add additional tests for enable/disable isolation