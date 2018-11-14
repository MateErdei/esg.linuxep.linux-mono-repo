/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockSensorDataCallback.h"

#include "TestExecutionSynchronizer.h"
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/PluginProtocol/Protocol.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/TestHelpers/TempDir.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <future>

#ifndef ARTISANBUILD


using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
using namespace Common::PluginApiImpl;
using namespace Common::PluginApi;
using Tests::TempDir;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Invoke;



class TrackSensorDataCallback: public Common::PluginApi::ISensorDataCallback
{

public:
    std::unordered_map<std::string, std::vector<std::string> > trackReceivedData;
    std::function<void(std::string)> m_extraCallback;
    explicit TrackSensorDataCallback(std::function<void(std::string)> extraCallback = [](std::string){}) :
        trackReceivedData(), m_extraCallback(std::move(extraCallback))
    {

    }
    void receiveData( const std::string & key, const std::string & data) override
    {
        auto found = trackReceivedData.find(key);
        if ( found == trackReceivedData.end())
        {
            trackReceivedData.emplace_hint(found, key, std::vector<std::string>{data});
        }
        else
        {
            found->second.emplace_back(data);
        }
        m_extraCallback(data);
    }
};

class PubSubTests : public ::testing::Test
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
        std::string tempdirPath = "ipc://" + tempDir->absPath("datachannel.ipc");
        MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
        MockedApplicationPathManager &mock(*mockAppManager);
        ON_CALL(mock, getPublisherDataChannelAddress()).WillByDefault(Return(tempdirPath));
        ON_CALL(mock, getSubscriberDataChannelAddress()).WillByDefault(Return(tempdirPath));
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));


    }
    void TearDown() override
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }
};
std::unique_ptr<TempDir> PubSubTests::tempDir;

TEST_F(PubSubTests, WhenSubscriberReconnectItShouldContinueToReceivePublications) // NOLINT
{
    PluginResourceManagement pluginResourceManagement;


    std::shared_ptr<TrackSensorDataCallback> trackBefore = std::make_shared<TrackSensorDataCallback>();
    std::shared_ptr<TrackSensorDataCallback> trackAfter = std::make_shared<TrackSensorDataCallback>();

    std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> sensorDataSubscriber = pluginResourceManagement.createSensorDataSubscriber("news", trackBefore);
    sensorDataSubscriber->start();

    auto future_pub = std::async(std::launch::async, [&pluginResourceManagement](){
        auto sensorDataPublisher =  pluginResourceManagement.createSensorDataPublisher("plugin");
        for( int i = 0 ; i< 1000; i++)
        {
            sensorDataPublisher->sendData("news", std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    // crash subscriber and return it.
    sensorDataSubscriber.reset();
    sensorDataSubscriber = pluginResourceManagement.createSensorDataSubscriber("news", trackAfter);
    sensorDataSubscriber->start();

    EXPECT_TRUE(future_pub.get());
    // expectations:
    EXPECT_EQ(trackBefore->trackReceivedData.size(), 1) ;
    std::vector<std::string> receivedData = trackBefore->trackReceivedData["news"];
    ASSERT_GT(receivedData.size(), 0);
    // every entry in the received data is the previous + 1
    int firstEntry = std::stoi(receivedData.at(0));
    for( size_t i =0; i< receivedData.size(); i++)
    {
        size_t expectedValue = firstEntry + i;
        std::string expectedValueString = std::to_string(expectedValue);
        EXPECT_EQ(expectedValueString, receivedData.at(i));
    }

    EXPECT_EQ(trackAfter->trackReceivedData.size(), 1) ;
    std::vector<std::string> receivedAfter = trackAfter->trackReceivedData["news"];
    ASSERT_GT(receivedAfter.size(), 0);
    // every entry in the received data is the previous + 1
    firstEntry = std::stoi(receivedAfter.at(0));
    for( size_t i =0; i< receivedAfter.size(); i++)
    {
        size_t expectedValue = firstEntry + i;
        std::string expectedValueString = std::to_string(expectedValue);
        EXPECT_EQ(expectedValueString, receivedAfter.at(i));
    }

    int lastEntryFirst = std::stoi(receivedData.at(receivedData.size()-1));
    int firstEntryLast = std::stoi(receivedAfter.at(0));
    EXPECT_LT( lastEntryFirst, firstEntryLast);
    int lastEntryLast = std::stoi(receivedAfter.at(receivedAfter.size()-1));
    EXPECT_GT(lastEntryLast, 990);

}


TEST_F(PubSubTests, SubscribersShouldContinueToReceiveDataIfPublishersCrashesAndComeBack)  // NOLINT
{
    PluginResourceManagement pluginResourceManagement;
    Tests::TestExecutionSynchronizer markReached;
    std::shared_ptr<TrackSensorDataCallback> trackBefore = std::make_shared<TrackSensorDataCallback>([&markReached](std::string data){
        if( std::stoi(data) == 150)
        {
            markReached.notify();
        }
    });

    std::unique_ptr<Common::PluginApi::ISensorDataSubscriber> sensorDataSubscriber = pluginResourceManagement.createSensorDataSubscriber("news", trackBefore);
    sensorDataSubscriber->start();


    auto sensorDataPublisher =  pluginResourceManagement.createSensorDataPublisher("plugin");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for( int i = 0 ; i< 100; i++)
    {
        sensorDataPublisher->sendData("news", std::to_string(i));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sensorDataPublisher.reset(); // simulation of crashes.
    sensorDataPublisher =  pluginResourceManagement.createSensorDataPublisher("plugin");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for( int i = 100 ; i< 200; i++)
    {
        sensorDataPublisher->sendData("news", std::to_string(i));
    }

    ASSERT_TRUE( markReached.waitfor(1000)); // demonstrate that subscriber received 150 which is after the crash. L
    sensorDataSubscriber.reset();
    // expectations:
    EXPECT_EQ(trackBefore->trackReceivedData.size(), 1) ;
    std::vector<std::string> receivedData = trackBefore->trackReceivedData["news"];
    ASSERT_GT(receivedData.size(), 0);

}

#endif