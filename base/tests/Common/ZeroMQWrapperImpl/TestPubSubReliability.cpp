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
#include <Common/ZeroMQWrapperImpl/SocketPublisherImpl.h>


#ifndef ARTISANBUILD
#ifdef SKIPTHISTESTSTEMP
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
        std::atomic<bool> stopSubscriber(false);
        auto fasterPublisher = std::async(std::launch::async, [serveraddress, &synchronizer, &stopPublisher]() {
            auto m_context = Common::ZeroMQWrapper::createContext();
            auto publisher = m_context->getPublisher();
            auto publisherimpl = dynamic_cast<Common::ZeroMQWrapperImpl::SocketPublisherImpl*>(publisher.get());

            // force hwm, but next test show that even without forcing it, eventually messages will be dropped.
            int hwm=10;
            int rc = zmq_setsockopt(publisherimpl->skt(), ZMQ_SNDHWM, &hwm, sizeof(hwm));
            ASSERT_TRUE(rc==0);

            publisher->listen(serveraddress);
            int limit = std::numeric_limits<int>::max();
            for(int i = 0; i< limit; i++)
            {
                if (stopPublisher) break;
                std::string value = std::to_string(i);
                publisher->write(data_t{"news", value});
            }
        });

        // simulate a replier that answers after the requester timeout.
        auto slowSubscriber = std::async(std::launch::async, [serveraddress, &synchronizer, & stopSubscriber]() ->std::vector<int> {
            auto m_context = Common::ZeroMQWrapper::createContext();
            auto subscriber = m_context->getSubscriber();

            subscriber->connect(serveraddress);
            subscriber->subscribeTo("news");
            std::vector<int> received;
            int count_jumps = 0;
            int previous = 0;
            while( !stopSubscriber )
            {
                data_t newdata = subscriber->read();
                int value = std::stoi( newdata.back());
                received.push_back(value);
                if( value != previous + 1)
                {
                    count_jumps++;
                }
                previous = value;
                // demonstrate that it will loose data but keeping up receiving new data as possible.
                if( value > 50  && count_jumps > 2)
                {
                    synchronizer.notify();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            return received;

        });

        EXPECT_TRUE(synchronizer.waitfor(10000));
        stopPublisher = true;

        fasterPublisher.get();
        std::cout << "Publisher stop" << std::endl;

        if( slowSubscriber.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout)
        {
            stopSubscriber = true;

            EXPECT_TRUE(false) << "Subscriber did not detected jump";
        }
        auto received = slowSubscriber.get();
        auto size = received.size();
        EXPECT_GT(size, 0);
        int first = received.front();
        int last = received.back();
        EXPECT_GT(last, first + size);
    }


    // this test passes, but may take more than 1 minute to overload all the buffers and
    // show subscribers loosing events. It also demonstrate how 'difficult' is to subscribers to
    // loose events.
    TEST_F(PubSubReliabilityTests, DISABLED_DemonstrateThatSlowSubscribersHaveANaturalHWM)  // NOLINT
    {
        std::string serveraddress = m_testContext.serverAddress();
        Tests::TestExecutionSynchronizer synchronizer;
        std::atomic<bool> stopPublisher(false);
        std::atomic<bool> stopSubscriber(false);
        auto fasterPublisher = std::async(std::launch::async, [serveraddress, &stopPublisher]() {
            auto m_context = Common::ZeroMQWrapper::createContext();
            auto publisher = m_context->getPublisher();
            publisher->listen(serveraddress);
            int limit = std::numeric_limits<int>::max() - 5;
            int i =0;
            while (true)
            {
                if (stopPublisher) break;
                std::string value = std::to_string(i++);
                publisher->write(data_t{"news", value});
                if( i > limit)
                {
                    i = 0 ;
                }
            }
        });

        auto slowSubscriber = std::async(std::launch::async, [serveraddress, &synchronizer, & stopSubscriber]()  {
            auto m_context = Common::ZeroMQWrapper::createContext();
            auto subscriber = m_context->getSubscriber();

            subscriber->connect(serveraddress);
            subscriber->subscribeTo("news");
            int count_jumps = 0;
            int previous = 0;
            while( !stopSubscriber )
            {
                data_t newdata = subscriber->read();
                int value = std::stoi( newdata.back());
                if( value != previous + 1)
                {
                    count_jumps++;
                }
                previous = value;
                // demonstrate that it will loose data but keeping up receiving new data as possible.
                if( count_jumps > 1)
                {
                    synchronizer.notify();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

        });
        slowSubscriber.get();
        stopPublisher = true;
        fasterPublisher.get();

    }



}
#endif
#endif