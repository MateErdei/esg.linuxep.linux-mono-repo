/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/ZeroMQWrapper/IProxy.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/PluginProtocol/Protocol.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/IContextPtr.h>
#include <Common/ZeroMQWrapperImpl/ContextImpl.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/EventTypesImpl/EventConverter.h>

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/EventTypesImpl/TestEventTypeHelper.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <future>
#include <Common/EventTypes/EventStrings.h>

#ifndef ARTISANBUILD

using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
using namespace Common::PluginApiImpl;
using namespace Common::PluginApi;
using Tests::TempDir;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Invoke;


class TrackSensorDataCallback : public virtual Common::PluginApi::IEventVisitorCallback
{

public:
    std::unordered_map<std::string, std::vector<int> > trackReceivedData;
    std::function<void(int)> m_extraCallback;
    std::mutex mutex;
    explicit TrackSensorDataCallback(std::function<void(int)> extraCallback = [](int){}) :
        trackReceivedData(), m_extraCallback(std::move(extraCallback))
    {

    }

    void receiveData(const std::string& key, const std::string& data) override
    {
    }

    void processEvent(const Common::EventTypes::CredentialEvent & event) override
    {
        // not using CredentialEvent for these tests as PortScanning is lighterweight
    }

    void processEvent(const Common::EventTypes::ProcessEvent & event) override
    {
        // not using ProcessEvent for these tests as PortScanning is lighterweight
    }

    void processEvent(const Common::EventTypes::PortScanningEvent & event) override
    {
        std::string eventTypeId = Common::EventTypes::PortScanningEventName;
        std::lock_guard guard{mutex};
        auto found = trackReceivedData.find(eventTypeId);
        if (found == trackReceivedData.end())
        {
            int portNumber = event.getConnection().sourceAddress.port;
            trackReceivedData.emplace_hint(found, eventTypeId, std::vector<int>{portNumber});
        }
        else
        {
            found->second.emplace_back(event.getConnection().sourceAddress.port);
        }
        m_extraCallback(event.getConnection().sourceAddress.port);
    }
};

class PubSubTests : public Tests::TestEventTypeHelper
{
public:
    static std::unique_ptr<TempDir> tempDir;

    static void SetUpTestCase()
    {
        tempDir = TempDir::makeTempDir();
    }

    static void TearDownTestCase()
    {
        tempDir.reset(nullptr);
    }

    void SetUp() override
    {
        std::string tempdirPubPath = "ipc://" + tempDir->absPath("datachannelpub.ipc");
        std::string tempdirSubPath = "ipc://" + tempDir->absPath("datachannelsub.ipc");
        m_context = std::make_shared<Common::ZeroMQWrapperImpl::ContextImpl>();
        m_proxy = m_context->getProxy( tempdirSubPath, tempdirPubPath);
        m_proxy->start();
        MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
        MockedApplicationPathManager &mock(*mockAppManager);
        ON_CALL(mock, getPublisherDataChannelAddress()).WillByDefault(Return(tempdirSubPath));
        ON_CALL(mock, getSubscriberDataChannelAddress()).WillByDefault(Return(tempdirPubPath));
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));


    }
    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }
    Common::ZeroMQWrapper::IContextSharedPtr m_context;
    Common::ZeroMQWrapper::IProxyPtr m_proxy;
};
std::unique_ptr<TempDir> PubSubTests::tempDir;

TEST_F(PubSubTests, PubSendsDataReceiverReceives)
{
    PluginResourceManagement pluginResourceManagement;
    auto sensorDataPublisher =  pluginResourceManagement.createRawDataPublisher();
    std::shared_ptr<TrackSensorDataCallback> trackAfter = std::make_shared<TrackSensorDataCallback>();
    Common::EventTypes::PortScanningEvent portevent = createDefaultPortScanningEvent();
    std::unique_ptr<Common::PluginApi::ISubscriber> sensorDataSubscriber = pluginResourceManagement.createSubscriber(Common::EventTypes::PortScanningEventName, trackAfter);
    sensorDataSubscriber->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto connection = portevent.getConnection();
    for( unsigned int i = 0 ; i< 1000; i++)
    {
        connection.sourceAddress.port=i;
        portevent.setConnection(connection);
        sensorDataPublisher->sendData(Common::EventTypes::PortScanningEventName,portevent.toString());
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    sensorDataSubscriber->stop();
    EXPECT_EQ(trackAfter->trackReceivedData.size(), 1) ;
}

TEST_F(PubSubTests, WhenSubscriberReconnectItShouldContinueToReceivePublications) // NOLINT
{
    PluginResourceManagement pluginResourceManagement;
    Common::EventTypes::PortScanningEvent portevent = createDefaultPortScanningEvent() ;
    auto connection = portevent.getConnection();

    std::shared_ptr<TrackSensorDataCallback> trackBefore = std::make_shared<TrackSensorDataCallback>();
    std::shared_ptr<TrackSensorDataCallback> trackAfter = std::make_shared<TrackSensorDataCallback>();

    std::unique_ptr<Common::PluginApi::ISubscriber> sensorDataSubscriber = pluginResourceManagement.createSubscriber(Common::EventTypes::PortScanningEventName, trackBefore);
    sensorDataSubscriber->start();
    auto future_pub = std::async(std::launch::async, [&pluginResourceManagement,&portevent,&connection](){
        auto sensorDataPublisher =  pluginResourceManagement.createRawDataPublisher();
        for( unsigned int i = 0 ; i< 1000; i++)
        {
            connection.sourceAddress.port=i;
            portevent.setConnection(connection);
            sensorDataPublisher->sendData(Common::EventTypes::PortScanningEventName,portevent.toString());
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    // crash subscriber and restart it.
    sensorDataSubscriber.reset();
    sensorDataSubscriber = pluginResourceManagement.createSubscriber(Common::EventTypes::PortScanningEventName, trackAfter);
    sensorDataSubscriber->start();
    EXPECT_TRUE(future_pub.get());
    sensorDataSubscriber.reset();

    // expectations:
    EXPECT_EQ(trackBefore->trackReceivedData.size(), 1) ;
    std::vector<int> receivedData = trackBefore->trackReceivedData[Common::EventTypes::PortScanningEventName];
    ASSERT_GT(receivedData.size(), 0);
    // every entry in the received data is the previous + 1
    int firstEntry = receivedData.at(0);
    for( size_t i =0; i< receivedData.size(); i++)
    {
        size_t expectedValue = firstEntry + i;
        EXPECT_EQ(expectedValue, receivedData.at(i));
    }

    EXPECT_EQ(trackAfter->trackReceivedData.size(), 1) ;
    std::vector<int> receivedAfter = trackAfter->trackReceivedData[Common::EventTypes::PortScanningEventName];
    ASSERT_GT(receivedAfter.size(), 0);
    // every entry in the received data is the previous + 1
    firstEntry = receivedAfter.at(0);
    for( size_t i =0; i< receivedAfter.size(); i++)
    {
        size_t expectedValue = firstEntry + i;
        EXPECT_EQ(expectedValue, receivedAfter.at(i));
    }

    int lastEntryFirst = receivedData.at(receivedData.size()-1);
    int firstEntryLast = receivedAfter.at(0);
    EXPECT_LT( lastEntryFirst, firstEntryLast);
    int lastEntryLast = receivedAfter.at(receivedAfter.size()-1);
    EXPECT_GT(lastEntryLast, 990);

}


TEST_F(PubSubTests, SubscribersShouldContinueToReceiveDataIfPublishersCrashesAndComeBack)  // NOLINT
{
    Common::Logging::ConsoleLoggingSetup loggingSetup;
    PluginResourceManagement pluginResourceManagement;
    Tests::TestExecutionSynchronizer markReached;
    std::shared_ptr<TrackSensorDataCallback> trackBefore = std::make_shared<TrackSensorDataCallback>([&markReached](int data){
        if( data == 150)
        {
            markReached.notify();
        }
    });

    Common::EventTypes::PortScanningEvent portevent = createDefaultPortScanningEvent() ;
    auto connection = portevent.getConnection();

    std::unique_ptr<Common::PluginApi::ISubscriber> sensorDataSubscriber = pluginResourceManagement.createSubscriber(Common::EventTypes::PortScanningEventName, trackBefore);
    sensorDataSubscriber->start();


    auto sensorDataPublisher =  pluginResourceManagement.createRawDataPublisher();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for( unsigned int i = 0 ; i< 100; i++)
    {
        connection.sourceAddress.port=i;
        portevent.setConnection(connection);
        sensorDataPublisher->sendData(Common::EventTypes::PortScanningEventName,portevent.toString());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sensorDataPublisher.reset(); // simulation of crashes.
    sensorDataPublisher =  pluginResourceManagement.createRawDataPublisher();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for( unsigned int i = 100 ; i< 200; i++)
    {
        connection.sourceAddress.port=i;
        portevent.setConnection(connection);
        sensorDataPublisher->sendData(Common::EventTypes::PortScanningEventName,portevent.toString());
    }

    ASSERT_TRUE( markReached.waitfor(1000)); // demonstrate that subscriber received 150 which is after the crash.
    sensorDataSubscriber.reset();
    // expectations:
    EXPECT_EQ(trackBefore->trackReceivedData.size(), 1) ;
    std::vector<int> receivedData = trackBefore->trackReceivedData[Common::EventTypes::PortScanningEventName];
    ASSERT_GT(receivedData.size(), 0);

}

#endif
