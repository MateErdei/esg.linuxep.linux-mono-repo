/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockSubscriberLib.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <SubscriberLib/Subscriber.h>
#include <gtest/gtest.h>
#include <modules/pluginimpl/Logger.h>
#include <pluginimpl/PluginAdapter.h>
#include <pluginimpl/QueueTask.h>

#include <future>

using namespace Common::FileSystem;

// Generic tests
TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}

TEST(TestLinkerWorks, WorksWithTheLogging) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");
}

// Test plugin adapter
class DummyServiceApi : public Common::PluginApi::IBaseServiceApi
{
public:
    void sendEvent(const std::string&, const std::string&) const override{};

    void sendStatus(const std::string&, const std::string&, const std::string&) const override{};

    void requestPolicies(const std::string&) const override{};
};

class TestablePluginAdapter : public Plugin::PluginAdapter
{
public:
    TestablePluginAdapter(
        std::shared_ptr<Plugin::QueueTask> queueTask,
        std::unique_ptr<SubscriberLib::ISubscriber> subscriber) :
        Plugin::PluginAdapter(
            queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi>(new DummyServiceApi()),
            std::make_shared<Plugin::PluginCallback>(queueTask),
            std::move(subscriber))
    {
    }
};

class PluginAdapterTests : public LogOffInitializedTests
{
};

TEST_F(PluginAdapterTests, PluginAdapterRestartsSubscriberIfItStops)
{
    int subscriberRunningStatusCall = 0;
    auto countStatusCalls = [&]()
    {
        subscriberRunningStatusCall++;
        return false;
    };

    MockSubscriberLib* mockSubscriber = new StrictMock<MockSubscriberLib>();
    EXPECT_CALL(*mockSubscriber, start).Times(1); // Plugin starting up subscriber
    EXPECT_CALL(*mockSubscriber, stop).Times(1);  // Plugin stopping subscriber on stop task
    EXPECT_CALL(*mockSubscriber, getRunningStatus).WillRepeatedly(Invoke(countStatusCalls));
    EXPECT_CALL(*mockSubscriber, restart).Times(1);
    std::unique_ptr<SubscriberLib::ISubscriber> mockSubscriberPtr(mockSubscriber);
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask, std::move(mockSubscriberPtr));
    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &pluginAdapter);
    while (subscriberRunningStatusCall == 0)
    {
        usleep(100);
    }
    queueTask->pushStop();
    mainLoopFuture.get();
}

TEST_F(PluginAdapterTests, PluginAdapterMainLoopThrowsIfSocketDirDoesNotExist)
{
    MockSubscriberLib* mockSubscriber = new StrictMock<MockSubscriberLib>();
    EXPECT_CALL(*mockSubscriber, start).WillOnce(Throw(std::runtime_error("Exception on start")));
    std::unique_ptr<SubscriberLib::ISubscriber> mockSubscriberPtr(mockSubscriber);
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask, std::move(mockSubscriberPtr));

    // Example exception on start: If the socket dir does not exist then the whole plugin will exit.
    // If that dir is missing then the installation is unusable and this plugin shouldn't try and fix it.
    // So throw and let WD start us up again if needed.
    EXPECT_THROW(pluginAdapter.mainLoop(), std::runtime_error);
}
