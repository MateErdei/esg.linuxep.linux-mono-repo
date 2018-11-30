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
#include <Common/TestHelpers/TestEventTypeHelper.h>

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

    std::shared_ptr<MockRawDataCallback> mockRawDataCallback;
    std::unique_ptr<Common::PluginApi::IRawDataPublisher> rawDataPublisher;
    std::unique_ptr<Common::PluginApi::ISubscriber> subscriber;
};

class FakePortScanningDealer : public AbstractRawDataCallback
{
public:

    Common::EventTypesImpl::PortScanningEvent m_event;

    void processEvent(Common::EventTypesImpl::PortScanningEvent& portScanningEvent)
    {
        m_event =  portScanningEvent;
    }
};

class FakeCredentialsDealer : public AbstractRawDataCallback
{
public:

    Common::EventTypesImpl::CredentialEvent m_event;

    void processEvent(Common::EventTypesImpl::CredentialEvent& credentialEvent)
    {
        m_event =  credentialEvent;
    }

};

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceiveData)
{
    Tests::TestExecutionSynchronizer testExecutionSynchronizer(2);
    Common::Threads::NotifyPipe notify;

    mockRawDataCallback = std::make_shared<NiceMock<MockRawDataCallback>>();
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
    while( credentialCallback->m_event.getGroupName() == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, credentialCallback->m_event);
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
    while( portScanningCallback->m_event.getConnection().destinationAddress.address == "")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30*(count+1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, portScanningCallback->m_event);
}