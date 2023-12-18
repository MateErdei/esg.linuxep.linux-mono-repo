// Copyright 2023 Sophos Limited. All rights reserved.

#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/TaskQueue.h"
#include "pluginimpl/config.h"

#include "Common/XmlUtilities/AttributesMap.h"

#include "base/tests/Common/Helpers/MemoryAppender.h"
#include "base/tests/Common/Helpers/MockApiBaseServices.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "base/modules/Common/ProcessImpl/ProcessImpl.h"

#include <gtest/gtest.h>

#include <future>
#include <memory>
#include <thread>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>

using namespace testing;

namespace
{
    class TestablePluginAdapter : public Plugin::PluginAdapter
    {
    public:
        explicit TestablePluginAdapter(
                const std::shared_ptr<Plugin::TaskQueue>& queueTask,
                std::shared_ptr<Common::PluginApi::IBaseServiceApi> baseServiceApi
        ) :
                Plugin::PluginAdapter(
                        queueTask,
                        std::move(baseServiceApi),
                        std::make_shared<Plugin::PluginCallback>(queueTask))
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
    auto queue = std::make_shared<Plugin::TaskQueue>();
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
    auto queue = std::make_shared<Plugin::TaskQueue>();
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
    auto queue = std::make_shared<Plugin::TaskQueue>();
    TestablePluginAdapter plugin(queue, mockBaseService);

    plugin.setQueueTimeout(std::chrono::milliseconds{1});
    plugin.setDebugTimeout(std::chrono::milliseconds {2});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &plugin);
    EXPECT_TRUE(waitForLog("Failed to get NTP policy within ", std::chrono::milliseconds{20}));
    queue->pushStop();
    mainLoopFuture.get();


    EXPECT_TRUE(status.empty());
}

TEST_F(PluginAdapterTests, pluginPutsRevidInStatus)
{
    constexpr const auto* REVID="ThisIsARevID";
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));
    auto queue = std::make_shared<Plugin::TaskQueue>();
    queue->push(Plugin::Task{
        .taskType = Plugin::Task::TaskType::Policy,
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

TEST_F(PluginAdapterTests, logsWhenIsolationEnabled)
{
    UsingMemoryAppender appender(*this);
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/deviceisolation/bin/nft")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isExecutable("/opt/sophos-spl/plugins/deviceisolation/bin/nft")).WillRepeatedly(Return(true));
    mode_t mode = static_cast<int>(std::filesystem::perms::owner_read) |
                  static_cast<int>(std::filesystem::perms::owner_write);
    EXPECT_CALL(*mockFileSystem, writeFileAtomically("/opt/sophos-spl/plugins/deviceisolation/var/nft_rules", _, "/opt/sophos-spl/plugins/deviceisolation/tmp", mode)).WillOnce(Return());
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();
                std::vector<std::string> args = { "-c", "-f", "/opt/sophos-spl/plugins/deviceisolation/var/nft_rules"};
                EXPECT_CALL(*mockProcess, exec("/opt/sophos-spl/plugins/deviceisolation/bin/nft", args)).Times(1);
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500))
                    .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));



                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });

    auto queue = std::make_shared<Plugin::TaskQueue>();
    queue->push(Plugin::Task{
       .taskType = Plugin::Task::TaskType::Action,
       .Content = R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>true</enabled>
    </action>)SOPHOS"
    });
    queue->pushStop();
    TestablePluginAdapter plugin(queue, mockBaseService);
    plugin.mainLoop();

    // Check we have logged
    ASSERT_TRUE(waitForLog("Enabling Device Isolation"));

    // Check status includes that we have isolated
    ASSERT_NE(status, "");
    auto attrmap = Common::XmlUtilities::parseXml(status);
    auto elements = attrmap.lookupMultiple("status/isolation");
    ASSERT_EQ(elements.size(), 1);
    auto comp = elements.at(0);
    EXPECT_EQ(comp.value("self"), "false");
    EXPECT_EQ(comp.value("admin"), "true");
}

TEST_F(PluginAdapterTests, logsWhenIsolationDisabled)
{
    UsingMemoryAppender appender(*this);
    auto mockBaseService = std::make_shared<StrictMock<MockApiBaseServices>>();
    EXPECT_CALL(*mockBaseService, requestPolicies("NTP")).Times(1);
    std::string status;
    EXPECT_CALL(*mockBaseService, sendStatus("NTP", _, _)).WillRepeatedly(SaveArg<2>(&status));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            []()
            {
                auto mockProcess = new StrictMock<MockProcess>();

                // Flush
                std::vector<std::string> args = { "flush", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec("/opt/sophos-spl/plugins/deviceisolation/bin/nft", args));
                EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 500)).WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillRepeatedly(Return(0));

                // Delete
                args = { "delete", "table", "inet", "sophos_device_isolation" };
                EXPECT_CALL(*mockProcess, exec("/opt/sophos-spl/plugins/deviceisolation/bin/nft", args));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            });


    auto queue = std::make_shared<Plugin::TaskQueue>();
    queue->push(Plugin::Task{
            .taskType = Plugin::Task::TaskType::Action,
            .Content = R"SOPHOS(<?xml version="1.0" ?>
    <action type="sophos.mgt.action.IsolationRequest">
         <enabled>false</enabled>
    </action>)SOPHOS"
    });
    queue->pushStop();
    TestablePluginAdapter plugin(queue, mockBaseService);
    plugin.mainLoop();

    // Check we have logged
    ASSERT_TRUE(waitForLog("Disabling Device Isolation"));

    // Check status includes that we have isolated
    ASSERT_NE(status, "");
    auto attrmap = Common::XmlUtilities::parseXml(status);
    auto elements = attrmap.lookupMultiple("status/isolation");
    ASSERT_EQ(elements.size(), 1);
    auto comp = elements.at(0);
    EXPECT_EQ(comp.value("self"), "false");
    EXPECT_EQ(comp.value("admin"), "false");
}