/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockEventVisitorCallback.h"
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

    void setupPubSub(std::string eventTypeId, std::shared_ptr<IRawDataCallback> rawCallback)
    {
        subscriber = pluginResourceManagement.createSubscriber(eventTypeId, rawCallback);
        subscriber->start();
        rawDataPublisher = pluginResourceManagement.createRawDataPublisher();
    }

    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
     PluginResourceManagement pluginResourceManagement;

    std::shared_ptr<MockEventVisitorCallback> mockRawDataCallback;
    std::unique_ptr<Common::PluginApi::IRawDataPublisher> rawDataPublisher;
    std::unique_ptr<Common::PluginApi::ISubscriber> subscriber;
};

class FakePortScanningDealer : public AbstractRawDataCallback
{
public:
    FakePortScanningDealer():m_event(Common::EventTypes::createEmptyPortScanningEvent()){}
    Common::EventTypes::IPortScanningEventPtr  m_event;

    void processEvent(Common::EventTypes::IPortScanningEventPtr portScanningEvent) override
    {
        m_event =  std::move(portScanningEvent);
    }
};

class FakeCredentialsDealer : public AbstractRawDataCallback
{
public:
    FakeCredentialsDealer():m_event(Common::EventTypes::createEmptyCredentialEvent()){}

    Common::EventTypes::ICredentialEventPtr m_event;

    void processEvent(Common::EventTypes::ICredentialEventPtr credentialEvent) override
    {
        m_event =  std::move(credentialEvent);
    }

};

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveData)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    mockRawDataCallback = std::make_shared<NiceMock<MockEventVisitorCallback>>();
    setupPubSub("Detector.Credentials",mockRawDataCallback);

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
        rawDataPublisher->sendPluginEvent(eventExpected3);
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

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveCredentialData)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakeCredentialsDealer> credentialCallback = std::make_shared<FakeCredentialsDealer>();
    setupPubSub("Detector.Credentials",credentialCallback);

    Common::EventTypesImpl::EventConverter converter;
    Common::EventTypesImpl::CredentialEvent eventExpected = createDefaultCredentialEvent();
    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    rawDataPublisher->sendData(data.first, data.second);

    int count = 0;
    while( credentialCallback->m_event->getGroupName() == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 20);
    }

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, *(credentialCallback->m_event));
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningData)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>();
    setupPubSub("Detector.PortScanning",portScanningCallback);

    Common::EventTypesImpl::EventConverter converter;
    Common::EventTypesImpl::PortScanningEvent eventExpected = createDefaultPortScanningEvent();
    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    rawDataPublisher->sendData(data.first, data.second);

    int count = 0;
    while( portScanningCallback->m_event->getConnection().destinationAddress.address == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 20);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, *(portScanningCallback->m_event));
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningDataUsingInterface)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>();
    setupPubSub("Detector.PortScanning",portScanningCallback);

    Common::EventTypesImpl::PortScanningEvent port;
    Common::EventTypes::IPortScanningEvent::EventType eventType = Common::EventTypesImpl::PortScanningEvent::opened;
    Common::EventTypes::IpFlow ipFlow = createDefaultIpFlow();
    std::unique_ptr<Common::EventTypes::IPortScanningEvent> eventExpected = Common::EventTypes::createPortScanningEvent(ipFlow,eventType);

    Common::EventTypesImpl::EventConverter converter;
    std::pair<std::string, std::string> data = converter.eventToString(eventExpected.get());
    rawDataPublisher->sendData(data.first, data.second);

    int count = 0;
    while( portScanningCallback->m_event->getConnection().destinationAddress.address == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 20);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, *eventExpected, *(portScanningCallback->m_event));
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningEventUsingInterface)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>();
    setupPubSub("Detector.PortScanning",portScanningCallback);

    Common::EventTypes::IPortScanningEvent::EventType eventType = Common::EventTypesImpl::PortScanningEvent::opened;
    Common::EventTypes::IpFlow ipFlow = createDefaultIpFlow();
    std::unique_ptr<Common::EventTypes::IPortScanningEvent> eventExpected = Common::EventTypes::createPortScanningEvent(ipFlow,eventType);

    rawDataPublisher->sendPluginEvent(*eventExpected);

    int count = 0;
    while( portScanningCallback->m_event->getConnection().destinationAddress.address == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 20);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, *eventExpected, *(portScanningCallback->m_event));
}

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveCredentialEventUsingInterface)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakeCredentialsDealer> credentialCallback = std::make_shared<FakeCredentialsDealer>();
    setupPubSub("Detector.Credentials",credentialCallback);

    Common::EventTypes::EventType eventType = Common::EventTypes::EventType::authFailure;
    Common::EventTypes::UserSid usersid ;
    usersid.username="TestUser";
    usersid.domain="stuff.com";

    std::unique_ptr<Common::EventTypes::ICredentialEvent> eventExpected = Common::EventTypes::createCredentialEvent(usersid,eventType);
    eventExpected->setSessionType(Common::EventTypes::SessionType::interactive);
    eventExpected->setLogonId(1000);
    eventExpected->setTimestamp(123123123);
    eventExpected->setGroupId(1001);
    eventExpected->setGroupName("TestGroup");

    Common::EventTypes::UserSid targetUserId;
    targetUserId.username = "TestTarUser";
    targetUserId.domain = "sophosTarget->com";
    eventExpected->setTargetUserSid(targetUserId);

    Common::EventTypes::NetworkAddress network;
    network.address = "sophos.com:400";
    eventExpected->setRemoteNetworkAccess(network);
    rawDataPublisher->sendPluginEvent(*eventExpected);

    int count = 0;
    while( credentialCallback->m_event->getSubjectUserSid().username == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        EXPECT_TRUE(count < 20);
        if (count > 20); break;
    }

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, *eventExpected, *(credentialCallback->m_event));
}