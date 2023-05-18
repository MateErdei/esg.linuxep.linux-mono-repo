// Copyright 2021-2023 Sophos Limited. All rights reserved.

// Class under test
#include "modules/SubscriberLib/Subscriber.h"

#include "modules/SubscriberLib/Logger.h"
#include "modules/Heartbeat/MockHeartbeatPinger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/WaitForUtils.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockZmqContext.h"

#include "MockEventQueuePusher.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <queue>

namespace
{
    class TestSubscriber : public LogOffInitializedTests
    {
        void TearDown() override
        {
            Tests::restoreFileSystem();
            Tests::restoreFilePermissions();
        }
    };
    class TestSubscriberWithLog : public LogInitializedTests
    {
    public:
        void TearDown() override
        {
            Tests::restoreFileSystem();
            Tests::restoreFilePermissions();
        }

        void setAppenderToLogWithTimestamps()
        {
            auto appenders = log4cplus::Logger::getRoot().getAllAppenders();
            for (const auto& appender : appenders)
            {
                std::unique_ptr<log4cplus::Layout> layout(
                    new log4cplus::PatternLayout("[%d{%S.%q}] %5p %m%n")); // NOLINT
                appender->setLayout(std::move(layout));
            }
        }
    };
}

TEST_F(TestSubscriber, SubscriberCanCallStopWithoutThrowingOnASubscriberThatHasntStarted) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventHandler> mockPusherPtr(mockPusher);
    auto heartbeatPinger =  std::make_shared<Heartbeat::HeartbeatPinger>();
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), heartbeatPinger, 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath))
        .WillOnce(Return(false))
        .WillOnce(Return(false));

    EXPECT_FALSE(subscriber.getRunningStatus());
    EXPECT_NO_THROW(subscriber.stop());
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriber, SubscriberStartAndStop) // NOLINT
{
    std::atomic<bool> readHasBeenCalled = false;

    auto countReadsAndReturnSimpleEvent = [&readHasBeenCalled](){
      readHasBeenCalled = true;
      return std::vector<std::string>{"threatEvents", "data"};
    };

    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/a/b/FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);
    EXPECT_CALL(*socketSubscriber, read()).WillRepeatedly(Invoke(countReadsAndReturnSimpleEvent));
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventHandler> mockPusherPtr(mockPusher);
    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), heartbeatPinger, 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath)))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(fakeSocketPath)).WillRepeatedly(Return());

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
        std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(1);

    EXPECT_FALSE(subscriber.getRunningStatus());
    subscriber.start();
    // Make sure we have really started, start launches a thread.
    while (!readHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(subscriber.getRunningStatus());
    EXPECT_NO_THROW(subscriber.stop());
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriberWithLog, SubscriberHandlesfailedChmod) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/a/b/FakeSocketPath";

    testing::internal::CaptureStderr();
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventHandler> mockPusherPtr(mockPusher);
    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), heartbeatPinger, 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath)))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath))
        .WillOnce(Return(false)) // initial check in start() called by test
        .WillOnce(Return(false)); // stop() call in destructor.

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
        std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    EXPECT_CALL(*mockFilePermissions,
                chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(1).WillRepeatedly(Throw(Common::FileSystem::IFileSystemException("thrown")));

    EXPECT_FALSE(subscriber.getRunningStatus());

    EXPECT_NO_THROW(subscriber.start());
    do
    {
        usleep(10);
    } while (subscriber.getRunningStatus());
    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errorMsg, ::testing::HasSubstr("Failed to set socket permissions: "));
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriber, SubscriberCanRestart)
{
    auto appenders = log4cplus::Logger::getRoot().getAllAppenders();
    for (const auto& appender : appenders)
    {
        std::unique_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout("[%d{%S.%q}] %5p %m%n")); // NOLINT
        appender->setLayout(std::move(layout));
    }

    auto context = std::make_shared<StrictMock<MockZmqContext>>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    auto mockPusherPtr = std::make_unique<NiceMock<MockEventQueuePusher>>();

    auto socketSubscriber = std::make_unique<StrictMock<MockSocketSubscriber>>();
    EXPECT_CALL(*socketSubscriber, setTimeout(1000)).Times(2);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(2);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(2);
    std::atomic_bool readHasBeenCalled = false;
    auto sleepAndReturnEmptyData = [&readHasBeenCalled](){
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        readHasBeenCalled = true;
        return std::vector<std::string>{"threatEvents", "data"};
    };
    EXPECT_CALL(*socketSubscriber, read()).WillRepeatedly(Invoke(sleepAndReturnEmptyData));

    context->m_subscriber = std::move(socketSubscriber);
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(std::move(context));

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();

    auto mockFileSystem = std::make_unique<NiceMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath)))
        .WillRepeatedly(Return(true));

    std::atomic_bool socketExists = true;
    auto checkSocketExists = [&socketExists](){
        return static_cast<bool>(socketExists);
    };
    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath)).WillRepeatedly(Invoke(checkSocketExists));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockFilePermissions = std::make_unique<StrictMock<MockFilePermissions>>();
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(2);
    Tests::replaceFilePermissions(std::move(mockFilePermissions));

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), heartbeatPinger, 1000);
    subscriber.start();

    while (!readHasBeenCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    readHasBeenCalled = false;
    EXPECT_TRUE(subscriber.getRunningStatus());
    subscriber.restart();

    readHasBeenCalled = false;
    while (!readHasBeenCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    EXPECT_TRUE(subscriber.getRunningStatus());
    socketExists = false;
    EXPECT_NO_THROW(subscriber.stop());
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriber, SubscriberStartThrowsIfSocketDirDoesNotExist) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    MockEventQueuePusher* mockPusher = new StrictMock<MockEventQueuePusher>();
    std::unique_ptr<IEventHandler> mockPusherPtr(mockPusher);

    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(0);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(0);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(0);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);
    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), heartbeatPinger, 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath))).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath)).WillOnce(Return(false));
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    testing::internal::CaptureStderr();
    EXPECT_FALSE(subscriber.getRunningStatus());
    EXPECT_THROW(subscriber.start(), std::runtime_error);
    EXPECT_FALSE(subscriber.getRunningStatus());
    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errorMsg, ::testing::HasSubstr("The events pub/sub socket directory does not exist:"));
}

TEST_F(TestSubscriber, SubscriberSendsDataToQueueWheneverItReceivesItFromTheSocket) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    MockEventQueuePusher* mockPusher = new StrictMock<MockEventQueuePusher>();
    std::unique_ptr<IEventHandler> mockPusherPtr(mockPusher);
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);

    std::vector<std::vector<std::string>> mockSocketValues = {
        {"threatEvents", "data1"},
        {"threatEvents", "data2"},
        {"threatEvents", "data3"},
        {"threatEvents", "data4"}
    };
    auto getNextEvent = [&mockSocketValues](){
        auto fakeEventData = mockSocketValues.back();
        mockSocketValues.pop_back();
        return fakeEventData;
    };
    EXPECT_CALL(*socketSubscriber, read()).WillRepeatedly(Invoke(getNextEvent));
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);
    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), heartbeatPinger, 123);

    auto mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath))).WillOnce(Return(true));

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
        std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(1);

    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath))
            .WillOnce(Return(false)) // initial check in start(), called by test
            .WillOnce(Return(true)) // The check before read, the first one we allow a read
            .WillOnce(Return(true)) // Second read read
            .WillOnce(Return(true)) // Third read
            .WillOnce(Return(true)) // Fourth read
            .WillOnce(Return(false)) // Next time around the read loop we fake the socket being missing here
            .WillOnce(Return(false)); // stop() call in destructor

    EXPECT_CALL(*mockPusher, handleEvent(JournalerCommon::Event{JournalerCommon::EventType::THREAT_EVENT, "data1"})).Times(1);
    EXPECT_CALL(*mockPusher, handleEvent(JournalerCommon::Event{JournalerCommon::EventType::THREAT_EVENT, "data2"})).Times(1);
    EXPECT_CALL(*mockPusher, handleEvent(JournalerCommon::Event{JournalerCommon::EventType::THREAT_EVENT, "data3"})).Times(1);
    EXPECT_CALL(*mockPusher, handleEvent(JournalerCommon::Event{JournalerCommon::EventType::THREAT_EVENT, "data4"})).Times(1);

    EXPECT_FALSE(subscriber.getRunningStatus());
    subscriber.start();
    do
    {
        usleep(10);
    } while (subscriber.getRunningStatus());
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriberWithLog, TestSubscriberPingsHeartbeatRepeatedly)
{
    setAppenderToLogWithTimestamps();

    std::string fakeSocketPath = "/fake/dir/for/socketPath";

    auto socketSubscriber = std::make_unique<NiceMock<MockSocketSubscriber>>();

    auto context = std::make_shared<StrictMock<MockZmqContext>>();
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    auto mockPusherPtr = std::make_unique<NiceMock<MockEventQueuePusher>>();
    auto mockHeartbeatPinger = std::make_shared<StrictMock<Heartbeat::MockHeartbeatPinger>>();

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), mockHeartbeatPinger, 123);

    auto mockFileSystem = std::make_unique<NiceMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath))).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath)).WillRepeatedly(Return(true));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockFilePermissions = std::make_unique<StrictMock<MockFilePermissions>>();
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(1);
    Tests::replaceFilePermissions(std::move(mockFilePermissions));

    EXPECT_FALSE(subscriber.getRunningStatus());
    EXPECT_CALL(*mockHeartbeatPinger, ping).Times(AtLeast(2));
    subscriber.start();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
}
