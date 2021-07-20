/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <modules/SubscriberLib/Subscriber.h>
#include <Common/Helpers/MockZmqContext.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockFilePermissions.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <queue>

#include "MockEventQueuePusher.h"

class TestSubscriber : public LogOffInitializedTests{};
class TestSubscriberWithLog : public LogInitializedTests{};


TEST_F(TestSubscriber, SubscriberCanCallStopWithoutThrowingOnASubscriberThatHasntStarted) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 123);

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
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/a/b/FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath)))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath))
        .WillOnce(Return(false)) // initial check in start() called by test
        .WillOnce(Return(false)) // stop() called by test
        .WillOnce(Return(false)); // stop() call in destructor.
    EXPECT_CALL(*mockFileSystem, removeFile(fakeSocketPath));

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
        std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(1);

    EXPECT_FALSE(subscriber.getRunningStatus());
    subscriber.start();
    EXPECT_TRUE(subscriber.getRunningStatus());
    subscriber.stop();
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
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 123);

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
    sleep(1);
    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errorMsg, ::testing::HasSubstr("Failed to set socket permissions: "));
    EXPECT_FALSE(subscriber.getRunningStatus());
}
TEST_F(TestSubscriber, SubscriberCanRestart) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);

    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(1000)).Times(2);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(2);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(2);

    auto sleepAndReturnEmptyData = [](){
        sleep(1);
        return std::vector<std::string>();
    };
    EXPECT_CALL(*socketSubscriber, read()).WillRepeatedly(Invoke(sleepAndReturnEmptyData));
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));

    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 1000);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath)))
        .WillOnce(Return(true))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath))
        .WillOnce(Return(false)) // initial check in start() called by test
        .WillOnce(Return(false)) // check in stop(), if it's still there it removes the socket.
        .WillOnce(Return(false)) // again, the check in start(), start was called by reset.
        .WillOnce(Return(true)) // reset checks socket exists after calling start()
        .WillOnce(Return(false)) // stop() call in test
        .WillOnce(Return(false)); // stop() call in destructor.
    EXPECT_CALL(*mockFileSystem, removeFile(fakeSocketPath)).Times(2);

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
        std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(2);

    subscriber.start();
    EXPECT_TRUE(subscriber.getRunningStatus());
    subscriber.restart();
    EXPECT_TRUE(subscriber.getRunningStatus());
    subscriber.stop();
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriber, SubscriberStartThrowsIfSocketDirDoesNotExist) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    MockEventQueuePusher* mockPusher = new StrictMock<MockEventQueuePusher>();
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);

    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(0);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(0);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(0);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 123);

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

TEST_F(TestSubscriber, SubscriberResetsIfSocketRemoved) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    MockEventQueuePusher* mockPusher = new NiceMock<MockEventQueuePusher>();
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);

    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);

    auto sleepAndReturnEmptyData = [](){
      sleep(1);
      return std::vector<std::string>();
    };
    EXPECT_CALL(*socketSubscriber, read()).WillRepeatedly(Invoke(sleepAndReturnEmptyData));
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(Common::FileSystem::dirName(fakeSocketPath))).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(fakeSocketPath))
        .WillOnce(Return(false)) // initial check in start(), called by test
        .WillOnce(Return(true)) // The check before read, the first one we allow a read
        .WillOnce(Return(false)) // Next time around the read loop we fake the socket being missing here
        .WillOnce(Return(false)); // stop() call in destructor

    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    EXPECT_CALL(*mockFilePermissions, chmod(fakeSocketPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).Times(1);

    EXPECT_FALSE(subscriber.getRunningStatus());
    subscriber.start();
    while (subscriber.getRunningStatus())
    {
        usleep(1000);
    }
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriber, SubscriberSendsDataToQueueWheneverItReceivesItFromTheSocket) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "/fake/dir/for/socketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    MockEventQueuePusher* mockPusher = new StrictMock<MockEventQueuePusher>();
    std::unique_ptr<IEventQueuePusher> mockPusherPtr(mockPusher);
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);

    std::vector<std::string> mockSocketValues = {"one", "two", "three", "four", "five"};
    auto sleepAndReturnData = [&mockSocketValues](){
        sleep(1);
        mockSocketValues.pop_back();
        return mockSocketValues;
    };
    EXPECT_CALL(*socketSubscriber, read()).WillRepeatedly(Invoke(sleepAndReturnData));
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr, std::move(mockPusherPtr), 123);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
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

    EXPECT_CALL(*mockPusher, push(Common::ZeroMQWrapper::data_t({"one", "two", "three", "four"}))).Times(1);
    EXPECT_CALL(*mockPusher, push(Common::ZeroMQWrapper::data_t({"one", "two", "three"}))).Times(1);
    EXPECT_CALL(*mockPusher, push(Common::ZeroMQWrapper::data_t({"one", "two"}))).Times(1);
    EXPECT_CALL(*mockPusher, push(Common::ZeroMQWrapper::data_t({"one"}))).Times(1);

    EXPECT_FALSE(subscriber.getRunningStatus());
    subscriber.start();
    while (subscriber.getRunningStatus())
    {
        usleep(1000);
    }
    EXPECT_FALSE(subscriber.getRunningStatus());
}
