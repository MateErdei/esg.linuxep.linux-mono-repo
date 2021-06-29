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
class TestableSubscriber: public SubscriberLib::Subscriber
{
public:
    TestableSubscriber(std::string address, Common::ZMQWrapperApi::IContextSharedPtr context, int timeout = 5000) :
        SubscriberLib::Subscriber(address,std::move(context),timeout)
    {
    }

    int m_timeout;

    void setSocketTimeout(int timeout)
    {
        m_timeout = timeout;
    }
    int getSocketTimeout()
    {
        return m_timeout;
    }

    std::vector<std::string> socketRead()
    {
            return std::vector<std::string>{};
    }
    void socketListen(){} // stub methods
    void socketSubscribe(const std::string& /*eventType*/){}  // stub methods
};
class TestSubscriber : public LogOffInitializedTests{};


TEST_F(TestSubscriber, test2) // NOLINT
{
    auto context = Common::ZMQWrapperApi::createContext();
    TestableSubscriber subscriber("socket-address", context,10);
    subscriber.start();
    EXPECT_TRUE(subscriber.getRunningStatus());
    sleep(1);
    subscriber.stop();
}

TEST_F(TestSubscriber, test1) // NOLINT
{

    MockZmqContext*  context = new StrictMock<MockZmqContext>();
//    MockZmqContext&  mock(*context);
//    ON_CALL(mock, setTimeout(5000));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber("will not actually create socket", mockContextPtr);
    subscriber.start();
}
