/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/EventTypes/EventStrings.h>
#include <Common/EventTypes/IEventConverter.h>
#include <Common/Helpers/PubSubPathReplacement.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/AbstractEventVisitor.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/common/LogInitializedTests.h>

#include <atomic>
#include <memory>
#include <thread>

using data_t = Common::ZeroMQWrapper::IReadable::data_t;
using namespace Common::PluginApi;

using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::StrictMock;

class RawDataCallbackTests : public LogInitializedTests
{
public:
    void TearDown() override
    {
        subscriber->stop();
        rawDataPublisher.reset();
        subscriber.reset();
    }

    void setupPubSub(const std::string& eventTypeId, std::shared_ptr<IEventVisitorCallback> rawCallback)
    {
        pluginResourceManagement = m_pathReplacement.createPluginResourceManagement();
        subscriber = pluginResourceManagement->createSubscriber(eventTypeId, std::move(rawCallback));
        subscriber->start();
        rawDataPublisher = pluginResourceManagement->createRawDataPublisher();
    }

    Tests::PubSubPathReplacement m_pathReplacement; // This provides data channels for testing
    std::unique_ptr<IPluginResourceManagement> pluginResourceManagement;

    std::unique_ptr<Common::PluginApi::IRawDataPublisher> rawDataPublisher;
    std::unique_ptr<Common::PluginApi::ISubscriber> subscriber;
};

class FakePortScanningDealer : public AbstractEventVisitor
{
public:
    explicit FakePortScanningDealer(Common::Threads::NotifyPipe& notify) :
        m_event(Common::EventTypes::PortScanningEvent()),
        m_eventReceived{ false },
        m_notify{ notify }
    {
    }
    Common::EventTypes::PortScanningEvent m_event;
    std::atomic<bool> m_eventReceived;
    Common::Threads::NotifyPipe m_notify;

    void processEvent(const Common::EventTypes::PortScanningEvent& portScanningEvent) override
    {
        if (m_eventReceived)
            return;

        m_event = portScanningEvent;
        m_eventReceived = true;
        m_notify.notify();
    }
};

TEST_F(RawDataCallbackTests, RawDataPublisher_SubscriberCanSendReceivePortScanningEventUsingInterface) // NOLINT
{
    Common::Threads::NotifyPipe notify;

    std::shared_ptr<FakePortScanningDealer> portScanningCallback = std::make_shared<FakePortScanningDealer>(notify);
    setupPubSub(Common::EventTypes::PortScanningEventName, portScanningCallback);

    Common::EventTypes::PortScanningEvent::EventType eventType = Common::EventTypes::PortScanningEvent::opened;
    Common::EventTypes::IpFlow ipFlow;
    ipFlow.sourceAddress.address = "192.168.0.1";
    ipFlow.sourceAddress.port = 80;
    ipFlow.destinationAddress.address = "192.168.0.2";
    ipFlow.destinationAddress.port = 80;
    ipFlow.protocol = 25;
    Common::EventTypes::PortScanningEvent eventExpected =
        Common::EventTypes::createPortScanningEvent(ipFlow, eventType);

    rawDataPublisher->sendPluginEvent(eventExpected);

    int count = 0;
    while (!notify.notified())
    {
        rawDataPublisher->sendPluginEvent(eventExpected);
        std::this_thread::sleep_for(std::chrono::milliseconds(3 * (count + 1)));
        count++;
        ASSERT_TRUE(count < 10);
    }

    ASSERT_EQ(
        eventExpected.getConnection().sourceAddress.port,
        portScanningCallback->m_event.getConnection().sourceAddress.port);
    ASSERT_EQ(
        eventExpected.getConnection().sourceAddress.address,
        portScanningCallback->m_event.getConnection().sourceAddress.address);
    ASSERT_EQ(
        eventExpected.getConnection().destinationAddress.port,
        portScanningCallback->m_event.getConnection().destinationAddress.port);
    ASSERT_EQ(
        eventExpected.getConnection().destinationAddress.address,
        portScanningCallback->m_event.getConnection().destinationAddress.address);
    ASSERT_EQ(eventExpected.getConnection().protocol, portScanningCallback->m_event.getConnection().protocol);
    ASSERT_EQ(eventExpected.getEventTypeId(), portScanningCallback->m_event.getEventTypeId());
    ASSERT_EQ("Detector.PortScanning", portScanningCallback->m_event.getEventTypeId());
}