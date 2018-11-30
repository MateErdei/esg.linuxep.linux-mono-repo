/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockRawDataCallback.h"
#include "TestExecutionSynchronizer.h"

#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/PluginProtocol/Protocol.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>


using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
using namespace Common::PluginApiImpl;
using namespace Common::PluginApi;

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Invoke;


class RawDataCallbackTests : public ::testing::Test
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

        mockRawDataCallback = std::make_shared<NiceMock<MockRawDataCallback>>();

        subscriber = pluginResourceManagement.createSubscriber("Detector.Credentials", mockRawDataCallback);
        subscriber->start();
        rawDataPublisher = pluginResourceManagement.createRawDataPublisher();
    }
    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        subscriber->stop();
        rawDataPublisher.reset();
        subscriber.reset();
    }

    Common::EventTypesImpl::CredentialEvent createDefaultCredentialEvent()
    {
        Common::EventTypesImpl::CredentialEvent event;
        event.setEventType(Common::EventTypes::EventType::authFailure);
        event.setSessionType(Common::EventTypes::SessionType::interactive);
        event.setLogonId(1000);
        event.setTimestamp(123123123);
        event.setGroupId(1001);
        event.setGroupName("TestGroup");

        Common::EventTypes::UserSid subjectUserId;
        subjectUserId.username = "TestSubUser";
        subjectUserId.domain = "sophos.com";
        event.setSubjectUserSid(subjectUserId);

        Common::EventTypes::UserSid targetUserId;
        targetUserId.username = "TestTarUser";
        targetUserId.domain = "sophosTarget.com";
        event.setTargetUserSid(targetUserId);

        Common::EventTypes::NetworkAddress network;
        network.address = "sophos.com:400";
        event.setRemoteNetworkAccess(network);

        return event;
    }

    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
     PluginResourceManagement pluginResourceManagement;

    std::shared_ptr<MockRawDataCallback> mockRawDataCallback;
    std::unique_ptr<Common::PluginApi::IRawDataPublisher> rawDataPublisher;
    std::unique_ptr<Common::PluginApi::ISubscriber> subscriber;
};

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveData)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    Common::EventTypesImpl::EventConverter converter;

    Common::EventTypesImpl::CredentialEvent eventExpected = createDefaultCredentialEvent();
    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    Common::EventTypesImpl::CredentialEvent eventExpected2 = createDefaultCredentialEvent();
    eventExpected2.setGroupId(1002);
    eventExpected2.setGroupName("TestGroup2");
    std::pair<std::string, std::string> data2 = converter.eventToString(&eventExpected2);

    Common::EventTypesImpl::CredentialEvent eventExpected3 = createDefaultCredentialEvent();
    eventExpected2.setGroupId(1003);
    eventExpected2.setGroupName("TestGroup3");
    std::pair<std::string, std::string> data3 = converter.eventToString(&eventExpected2);

    EXPECT_CALL(*mockRawDataCallback, receiveData(data.first, data.second)).Times(AtLeast(1)).WillRepeatedly(
            Invoke([&notify](const std::string &, const std::string & ){notify.notify();})
    );

    EXPECT_CALL(*mockRawDataCallback, receiveData("Detector.Credentials2", _ )).Times(2).WillRepeatedly(
            Invoke([&testExecutionSynchronizer](const std::string &, const std::string & ){testExecutionSynchronizer.notify();})
    );
    /*It is documented that the subscriber will not receive data if the publisher sends data
     * before the subscription is in place. Hence, the test has a sync phase. In it the subscription is first ensured to be
     * working via multiples delay to receive the first message 'ensureSubscriptionStarted'
     */
    int count = 0;
    while( !notify.notified())
    {
        rawDataPublisher->sendData(data.first, data.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    rawDataPublisher->sendData("otherArgument", "willNotBeReceived");
    rawDataPublisher->sendData("Detector.Credentials2", data2.second);
    rawDataPublisher->sendData("anyOtherKey", "willNotBeReceived");
    rawDataPublisher->sendData("Detector.Credentials2", data3.second);

    EXPECT_NO_THROW(testExecutionSynchronizer.waitfor(3000));
}