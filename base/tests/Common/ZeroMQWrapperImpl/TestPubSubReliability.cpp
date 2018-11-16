/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ZeroMQWrapper/IReadable.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <tests/Common/TestHelpers/TempDir.h>
#include <tests/Common/TestHelpers/TestExecutionSynchronizer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <future>
#include <zmq.h>


#ifndef ARTISANBUILD

namespace
{
    using data_t = Common::ZeroMQWrapper::IReadable::data_t;


    class TestContext
    {
        Tests::TempDir m_tempDir;
    public:
        TestContext()
        : m_tempDir( "/tmp", "pubsub")
        {

        }
        std::string killChannel() const
        {
            return "ipc://" + m_tempDir.absPath("killchannel.ipc");
        }
        std::string serverAddress() const
        {
            return "ipc://" + m_tempDir.absPath("server.ipc");
        }
    };



    class PubSubReliabilityTests
            : public ::testing::Test
    {
    public:
        TestContext m_testContext;
        PubSubReliabilityTests() :
        m_testContext()
        {

        }

    };


    TEST_F(PubSubReliabilityTests, SlowSubscribersMayLooseMessagesButWillStillReceiveMessagesFromThePublisher)  // NOLINT
    {

        std::string serveraddress = m_testContext.serverAddress();
        Tests::TestExecutionSynchronizer synchronizer;
        std::atomic<bool> stopPublisher(false);
        auto fasterPublisher = std::async(std::launch::async, [serveraddress, &synchronizer, &stopPublisher]() {
            std::unique_ptr<Common::ZeroMQWrapper::IContext> m_context = Common::ZeroMQWrapper::createContext();
            auto publisher = m_context->getPublisher();
            publisher->listen(serveraddress);

            for(int i = 0; i< 100000000; i++)
            {
                if (stopPublisher) break;
                std::string value = std::to_string(i);
                publisher->write(data_t{"news", value});
            }
        });

        // simulate a replier that answers after the requester timeout.
        auto slowSubscriber = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            std::unique_ptr<Common::ZeroMQWrapper::IContext> m_context = Common::ZeroMQWrapper::createContext();
            auto subscriber = m_context->getSubscriber();
            auto subscriberimpl = dynamic_cast<Common::ZeroMQWrapperImpl::SocketSubscriberImpl*>(subscriber.get());
            ASSERT_TRUE(subscriberimpl);
            int hwm=10;
            int rc = zmq_setsockopt(subscriberimpl->skt(), ZMQ_RCVHWM, &hwm, sizeof(hwm));
            ASSERT_TRUE(rc==0);

            subscriber->connect(serveraddress);
            subscriber->subscribeTo("news");

            int count_jumps = 0;
            int previous = 0;
            while( true )
            {
                data_t newdata = subscriber->read();
                int value = std::stoi( newdata.back());
                if( value != previous + 1)
                {
                    count_jumps++;
                    previous = value;
                }
                // demonstrate that it will loose data but keeping up receiving new data as possible.
                if( value > 1000  && count_jumps > 2)
                {
                    synchronizer.notify();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });


        EXPECT_TRUE(synchronizer.waitfor(3000));
        stopPublisher = true;
        slowSubscriber.get();
        fasterPublisher.get();
    }


}
#endif