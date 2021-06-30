/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/SubscriberLib/Subscriber.h>
//#include <Common/Helpers/MockZmqContext.h>
#include "MockZmqContext.h"
#include <Common/FileSystem/IFileSystem.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>

//
//class TestableSubscriber: public SubscriberLib::Subscriber
//{
//public:
//    TestableSubscriber(std::string address, Common::ZMQWrapperApi::IContextSharedPtr context, int timeout = 5000) :
//        SubscriberLib::Subscriber(address,std::move(context),timeout)
//    {
//    }
//
//    int m_timeout;
//
//    void setSocketTimeout(int timeout)
//    {
//        m_timeout = timeout;
//    }
//    int getSocketTimeout()
//    {
//        return m_timeout;
//    }
//
//    std::vector<std::string> socketRead()
//    {
//            return std::vector<std::string>{};
//    }
//    void socketListen(){} // stub methods
//    void socketSubscribe(const std::string& /*eventType*/){}  // stub methods
//};

class TestSubscriber : public LogOffInitializedTests{};


//TEST_F(TestSubscriber, test2) // NOLINT
//{
//    auto context = Common::ZMQWrapperApi::createContext();
//    TestableSubscriber subscriber("socket-address", context,10);
//    subscriber.start();
//    EXPECT_TRUE(subscriber.getRunningStatus());
//    sleep(1);
//    subscriber.stop();
//}

TEST_F(TestSubscriber, SubscriberStart) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
//    EXPECT_CALL(*mockFileSystem, )
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr,123);
    EXPECT_FALSE(subscriber.getRunningStatus());
    subscriber.start();
    EXPECT_TRUE(subscriber.getRunningStatus());
//    subscriber.stop();
}

TEST_F(TestSubscriber, SubscriberCanCallStopWithoutThrowingOnASubscriberThatHasntStarted) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);
    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr,123);
    EXPECT_FALSE(subscriber.getRunningStatus());
    EXPECT_NO_THROW(subscriber.stop());
    EXPECT_FALSE(subscriber.getRunningStatus());
}


TEST_F(TestSubscriber, SubscriberStartAndStop) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(1);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(1);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(1);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));

    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr,123);
    subscriber.start();
    EXPECT_TRUE(subscriber.getRunningStatus());
    subscriber.stop();
    EXPECT_FALSE(subscriber.getRunningStatus());
}

TEST_F(TestSubscriber, SubscriberCanRestart) // NOLINT
{
    MockZmqContext*  context = new StrictMock<MockZmqContext>();
    std::string fakeSocketPath = "FakeSocketPath";
    MockSocketSubscriber*  socketSubscriber = new StrictMock<MockSocketSubscriber>();
    EXPECT_CALL(*socketSubscriber, setTimeout(123)).Times(2);
    EXPECT_CALL(*socketSubscriber, listen("ipc://" + fakeSocketPath)).Times(2);
    EXPECT_CALL(*socketSubscriber, subscribeTo("threatEvents")).Times(2);
    context->m_subscriber = Common::ZeroMQWrapper::ISocketSubscriberPtr(std::move(socketSubscriber));

    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber(fakeSocketPath, mockContextPtr,123);
    subscriber.start();
    EXPECT_TRUE(subscriber.getRunningStatus());
    subscriber.reset();
    EXPECT_TRUE(subscriber.getRunningStatus());
}