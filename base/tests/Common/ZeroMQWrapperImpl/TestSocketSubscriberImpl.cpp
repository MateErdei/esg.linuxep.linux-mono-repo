/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Exceptions/Print.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <gtest/gtest.h>

#include <thread>

using Common::ZeroMQWrapper::ISocketSubscriberPtr;

namespace
{
    TEST(TestSocketSubscriberImpl, creation) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        EXPECT_NE(socket.get(), nullptr);
    }

    // cppcheck-suppress syntaxError
    TEST(TestSocketSubscriberImpl, listen) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        ASSERT_NE(socket.get(), nullptr);
        socket->listen("inproc://transferTest");
        socket->subscribeTo("FOOBAR");
    }

    class SenderThread
    {
    public:
        explicit SenderThread(Common::ZMQWrapperApi::IContext& context);
        ~SenderThread()
        {
            m_stopThread = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }
        void start();

    private:
        Common::ZMQWrapperApi::IContext& m_context;
        std::thread m_thread;
        bool m_stopThread;
        void run();
    };

    SenderThread::SenderThread(Common::ZMQWrapperApi::IContext& context) :
        m_context(context),
        m_thread(),
        m_stopThread(false)
    {
    }

    void SenderThread::start() { m_thread = std::thread(&SenderThread::run, this); }

    void SenderThread::run()
    {
        auto sender = m_context.getPublisher();

        // Start sender thread - since we need to wait for subscription to propagate
        sender->connect("inproc://transferTest");

        while (!m_stopThread)
        {
            try
            {
                sender->write({ "FOOBAR", "DATA" });
            }
            catch (const Common::ZeroMQWrapper::IIPCException& e)
            {
                PRINT("Failed to send subscription data: " << e.what());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    TEST(TestSocketSubscriberImpl, comms) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        ASSERT_NE(socket.get(), nullptr);
        socket->listen("inproc://transferTest");
        socket->subscribeTo("FOOBAR");

        // Start sender thread - since we need to wait for subscription to propagate
        SenderThread thread(*context);
        thread.start();

        auto data = socket->read();
        EXPECT_EQ(data.at(0), "FOOBAR");
        EXPECT_EQ(data.at(1), "DATA");
    }

} // namespace
