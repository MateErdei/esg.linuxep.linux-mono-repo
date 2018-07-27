/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MockSensorDataCallback.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/PluginProtocol/MessageBuilder.h"
#include "Common/PluginProtocol/Protocol.h"
#include "Common/ZeroMQWrapper/IContext.h"
#include "TestExecutionSynchronizer.h"
#include "Common/Threads/NotifyPipe.h"
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <thread>



using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
using namespace Common::PluginApiImpl;
using namespace Common::PluginApi;

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Invoke;



class SensorDataCallbackTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
        MockedApplicationPathManager &mock(*mockAppManager);
        ON_CALL(mock, getPublisherDataChannelAddress()).WillByDefault(Return("inproc://datachannel.ipc"));
        ON_CALL(mock, getSubscriberDataChannelAddress()).WillByDefault(Return("inproc://datachannel.ipc"));
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

        mockSensorDataCallback = std::make_shared<NiceMock<MockSensorDataCallback>>();

        sensorDataSubscriber = pluginResourceManagement.createSensorDataSubscriber("news", mockSensorDataCallback);
        sensorDataSubscriber->start();
        sensorDataPublisher = pluginResourceManagement.createSensorDataPublisher("plugin");
    }
    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        sensorDataSubscriber->stop();
        sensorDataPublisher.reset();
        sensorDataSubscriber.reset();
    }

     PluginResourceManagement pluginResourceManagement;

    std::shared_ptr<MockSensorDataCallback> mockSensorDataCallback;
    std::unique_ptr<Common::PluginApi::ISensorDataPublisher> sensorDataPublisher;
    std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> sensorDataSubscriber;
};

TEST_F(SensorDataCallbackTests, SensorDataPublisher_SubscriberCanSendReceiveData)
{
    using ::testing::Invoke;
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    EXPECT_CALL(*mockSensorDataCallback, receiveData("news", "ensureSubscriptionStarted")).Times(AtLeast(1)).WillRepeatedly(
            Invoke([&notify](const std::string &, const std::string & ){notify.notify();})
    );

    EXPECT_CALL(*mockSensorDataCallback, receiveData("newsLetter", _ )).Times(2).WillRepeatedly(
            Invoke([&testExecutionSynchronizer](const std::string &, const std::string & ){testExecutionSynchronizer.notify();})
    );
    /*It is documented that the subscriber will not receive data if the publisher sends data
     * before the subscription is in place. Hence, the test has a sync phase. In it the subscription is first ensured to be
     * working via multiples delay to receive the first message 'ensureSubscriptionStarted'
     */
    int count = 0;
    while( !notify.notified())
    {
        sensorDataPublisher->sendData("news", "ensureSubscriptionStarted");
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    sensorDataPublisher->sendData("otherArgument", "willNotBeReceived");
    sensorDataPublisher->sendData("newsLetter", "firstnews");
    sensorDataPublisher->sendData("anyOtherKey", "willNotBeReceived");
    sensorDataPublisher->sendData("newsLetter", "secondnews");

    testExecutionSynchronizer.waitfor(3000);

}