//
// Created by pair on 08/06/18.
//



#include <gtest/gtest.h>

#include <Common/ZeroMQ_wrapper/ISocketSubscriber.h>
#include <Common/ZeroMQ_wrapper/ISocketPublisher.h>
#include <Common/ZeroMQ_wrapper/IContext.h>
#include <Common/ZeroMQ_wrapper/IContextPtr.h>

#include <thread>

using Common::ZeroMQ_wrapper::ISocketSubscriberPtr;

namespace
{
    TEST(TestSocketSubscriberImpl, creation) // NOLINT
    {
        Common::ZeroMQ_wrapper::IContextPtr context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        EXPECT_NE(socket.get(), nullptr);
    }

    TEST(TestSocketSubscriberImpl, listen) // NOLINT
    {
        Common::ZeroMQ_wrapper::IContextPtr context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        ASSERT_NE(socket.get(), nullptr);
        socket->listen("inproc://transferTest");
        socket->subscribeTo("FOOBAR");
    }

    class SenderThread
    {
    public:
        explicit SenderThread(Common::ZeroMQ_wrapper::IContext& context);
        ~SenderThread() {
            m_stopThread = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }
        void start();
    private:
        Common::ZeroMQ_wrapper::IContext& m_context;
        std::thread m_thread;
        bool m_stopThread;
        void run();
    };

    SenderThread::SenderThread(Common::ZeroMQ_wrapper::IContext &context)
        : m_context(context),
          m_thread(),
          m_stopThread(false)
    {
    }

    void SenderThread::start()
    {
        m_thread = std::thread(&SenderThread::run,this);
    }

    void SenderThread::run()
    {
        auto sender = m_context.getPublisher();

        // Start sender thread - since we need to wait for subscription to propagate
        sender->connect("inproc://transferTest");

        while (!m_stopThread)
        {
            sender->write({"FOOBAR", "DATA"});
            std::this_thread::sleep_for( std::chrono::milliseconds(10) );
        }
    }

    TEST(TestSocketSubscriberImpl, comms) // NOLINT
    {
        Common::ZeroMQ_wrapper::IContextPtr context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        ASSERT_NE(socket.get(), nullptr);
        socket->listen("inproc://transferTest");
        socket->subscribeTo("FOOBAR");

        auto sender = context->getPublisher();
        ASSERT_NE(sender.get(),nullptr);

        // Start sender thread - since we need to wait for subscription to propagate
        SenderThread thread(*context);
        thread.start();

        auto data = socket->read();
        EXPECT_EQ(data.at(0),"FOOBAR");
        EXPECT_EQ(data.at(1),"DATA");
    }

}
