/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockedApplicationPathManager.h"
#include "MockSensorDataCallback.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/PluginApi/IPluginApi.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/PluginApiImpl/MessageBuilder.h"
#include "Common/PluginApiImpl/Protocol.h"
#include "Common/ZeroMQWrapper/IContext.h"
#include "TestExecutionSynchronizer.h"

#include <chrono>
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
        pluginResourceManagement.setDefaultConnectTimeout(3000);

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

    EXPECT_CALL(*mockSensorDataCallback, receiveData("news", _ )).Times(2).WillRepeatedly(
            Invoke([&testExecutionSynchronizer](const std::string &, const std::string & ){testExecutionSynchronizer.notify();})
    );

    sensorDataPublisher->sendData("otherArgument", "willNotBeReceived");
    sensorDataPublisher->sendData("news", "firstnews");
    sensorDataPublisher->sendData("anyOtherKey", "willNotBeReceived");
    sensorDataPublisher->sendData("news", "secondnews");

    testExecutionSynchronizer.waitfor();

}