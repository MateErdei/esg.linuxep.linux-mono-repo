/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestExecutionSynchronizer.h"

#include <Common/EventTypes/IEventConverter.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/AbstractEventVisitor.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/PluginProtocol/Protocol.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/TestHelpers/TestEventTypeHelper.h>

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <memory>


using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
using namespace Common::PluginApiImpl;
using namespace Common::PluginApi;

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Invoke;


class RawDataCallbackTests : public Tests::TestEventTypeHelper
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


    }
    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        subscriber->stop();
        rawDataPublisher.reset();
        subscriber.reset();
    }

    void setupPubSub(const std::string& eventTypeId, std::shared_ptr<IEventVisitorCallback> rawCallback)
    {
        subscriber = pluginResourceManagement.createSubscriber(eventTypeId, std::move(rawCallback));
        subscriber->start();
        rawDataPublisher = pluginResourceManagement.createRawDataPublisher();
    }

    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
    PluginResourceManagement pluginResourceManagement;

    std::shared_ptr<IEventVisitorCallback> mockRawDataCallback;
    std::unique_ptr<Common::PluginApi::IRawDataPublisher> rawDataPublisher;
    std::unique_ptr<Common::PluginApi::ISubscriber> subscriber;
};

class FakePortScanningDealer : public AbstractEventVisitor
{
public:
    explicit FakePortScanningDealer(Common::Threads::NotifyPipe & notify)
        : m_event(Common::EventTypes::PortScanningEvent()), m_eventReceived{false}, m_notify{notify} {}
    Common::EventTypes::PortScanningEvent  m_event;
    std::atomic<bool> m_eventReceived;
    Common::Threads::NotifyPipe m_notify;

    void processEvent(Common::EventTypes::PortScanningEvent portScanningEvent) override
    {
        if( m_eventReceived) return;

        m_event = portScanningEvent;
        m_eventReceived = true;
        m_notify.notify();
    }
};

class FakeCredentialsDealer : public AbstractEventVisitor
{
public:
    explicit FakeCredentialsDealer(Common::Threads::NotifyPipe & notify)
            : m_event(Common::EventTypes::CredentialEvent()), m_eventReceived{false}, m_notify{notify} {}

    Common::EventTypes::CredentialEvent m_event;
    std::atomic<bool> m_eventReceived;
    Common::Threads::NotifyPipe& m_notify;

    void processEvent(Common::EventTypes::CredentialEvent credentialEvent) override
    {
        if( m_eventReceived) return;

        m_event = credentialEvent;
        m_eventReceived = true;
        m_notify.notify();
    }

};

class FakeDataDealer : public AbstractEventVisitor
{
public:
    explicit FakeDataDealer(Common::Threads::NotifyPipe & notify)
            :m_dataReceived{false}, m_notify{notify} {}

    std::atomic<bool> m_dataReceived;
    std::string m_data;
    Common::Threads::NotifyPipe& m_notify;

    void receiveData(const std::string& key, const std::string& data) override{
        if( m_dataReceived) return;

        m_data = data;
        m_dataReceived = true;
        m_notify.notify();
    }
};

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveData) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakeCredentialsDealer> credentialCallback = std::make_shared<FakeCredentialsDealer>(notify);
    setupPubSub("Detector.Credentials",credentialCallback);

    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();

    Common::EventTypes::CredentialEvent eventExpected = createDefaultCredentialEvent();
    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    Common::EventTypes::CredentialEvent eventExpected2 = createDefaultCredentialEvent();
    eventExpected2.setGroupId(1002);
    eventExpected2.setGroupName("TestGroup2");
    std::pair<std::string, std::string> data2 = converter->eventToString(&eventExpected2);

    Common::EventTypes::CredentialEvent eventExpected3 = createDefaultCredentialEvent();
    eventExpected2.setGroupId(1003);
    eventExpected2.setGroupName("TestGroup3");
    std::pair<std::string, std::string> data3 = converter->eventToString(&eventExpected2);

    /*It is documented that the subscriber will not receive data if the publisher sends data
     * before the subscription is in place. Hence, the test has a sync phase. In it the subscription is first ensured to be
     * working via multiples delay to receive the first message 'ensureSubscriptionStarted'
     */
    int count = 0;
    while( !notify.notified())
    {
        rawDataPublisher->sendData(data.first, data.second);
        rawDataPublisher->sendPluginEvent(eventExpected3);
        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    rawDataPublisher->sendData("otherArgument", "willNotBeReceived");
    rawDataPublisher->sendData("Detector.Credentials2", data2.second);
    rawDataPublisher->sendData("anyOtherKey", "willNotBeReceived");
    rawDataPublisher->sendData("Detector.Credentials2", data3.second);

    EXPECT_NO_THROW(testExecutionSynchronizer.waitfor(3000)); //NOLINT
}
TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveRawData) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::string random = "RandomString";
    std::shared_ptr<FakeDataDealer> rawDataCallback = std::make_shared<FakeDataDealer>(notify);
    setupPubSub("RawData",rawDataCallback);

    std::pair<std::string, std::string> data;
    data.first="RawData";
    data.second= random;

    int count = 0;
    while( !notify.notified())
    {
        rawDataPublisher->sendData(data.first, data.second);

        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_NO_THROW(testExecutionSynchronizer.waitfor(3000)); //NOLINT
    EXPECT_EQ(random,rawDataCallback->m_data);
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveCredentialData) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakeCredentialsDealer> credentialCallback = std::make_shared<FakeCredentialsDealer>(notify);
    setupPubSub("Detector.Credentials",credentialCallback);

    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    Common::EventTypes::CredentialEvent eventExpected = createDefaultCredentialEvent();
    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    rawDataPublisher->sendData(data.first, data.second);

    int count = 0;
    while( !notify.notified())
    {
        rawDataPublisher->sendData(data.first, data.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, credentialCallback->m_event);
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningData) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>(notify);
    setupPubSub("Detector.PortScanning",portScanningCallback);

    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    Common::EventTypes::PortScanningEvent eventExpected = createDefaultPortScanningEvent();
    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    rawDataPublisher->sendData(data.first, data.second);

    int count = 0;
    while(!notify.notified())
    {
        rawDataPublisher->sendData(data.first, data.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, portScanningCallback->m_event);
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningDataUsingInterface) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>(notify);
    setupPubSub("Detector.PortScanning",portScanningCallback);

    Common::EventTypes::PortScanningEvent port;
    Common::EventTypes::PortScanningEvent::EventType eventType = Common::EventTypes::PortScanningEvent::opened;
    Common::EventTypes::IpFlow ipFlow = createDefaultIpFlow();
    Common::EventTypes::PortScanningEvent eventExpected = Common::EventTypes::createPortScanningEvent(ipFlow,eventType);

    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);
    rawDataPublisher->sendData(data.first, data.second);

    int count = 0;
    while(!notify.notified())
    {
        rawDataPublisher->sendData(data.first, data.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, portScanningCallback->m_event);
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningEventUsingInterface) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>(notify);
    setupPubSub("Detector.PortScanning",portScanningCallback);

    Common::EventTypes::PortScanningEvent::EventType eventType = Common::EventTypes::PortScanningEvent::opened;
    Common::EventTypes::IpFlow ipFlow = createDefaultIpFlow();
    Common::EventTypes::PortScanningEvent eventExpected = Common::EventTypes::createPortScanningEvent(ipFlow,eventType);

    rawDataPublisher->sendPluginEvent(eventExpected);

    int count = 0;
    while(!notify.notified())
    {
        rawDataPublisher->sendPluginEvent(eventExpected);
        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, portScanningCallback->m_event);
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveCredentialEventUsingInterface) //NOLINT
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakeCredentialsDealer> credentialCallback = std::make_shared<FakeCredentialsDealer>(notify);
    setupPubSub("Detector.Credentials",credentialCallback);

    Common::EventTypes::CredentialEvent::EventType eventType = Common::EventTypes::CredentialEvent::EventType::authFailure;
    Common::EventTypes::UserSid usersid ;
    usersid.username="TestUser";
    usersid.domain="stuff.com";

    Common::EventTypes::CredentialEvent eventExpected = Common::EventTypes::createCredentialEvent(usersid,eventType);
    eventExpected.setSessionType(Common::EventTypes::CredentialEvent::SessionType::interactive);
    eventExpected.setLogonId(1000);
    eventExpected.setTimestamp(123123123);
    eventExpected.setGroupId(1001);
    eventExpected.setGroupName("TestGroup");

    Common::EventTypes::UserSid targetUserId;
    targetUserId.username = "TestTarUser";
    targetUserId.domain = "sophosTarget.com";
    eventExpected.setTargetUserSid(targetUserId);

    Common::EventTypes::NetworkAddress network;
    network.address = "sophos.com:400";
    eventExpected.setRemoteNetworkAccess(network);
    rawDataPublisher->sendPluginEvent(eventExpected);

    int count = 0;
    while(!notify.notified())
    {
        rawDataPublisher->sendPluginEvent(eventExpected);
        std::this_thread::sleep_for(std::chrono::milliseconds(3*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, credentialCallback->m_event);
}