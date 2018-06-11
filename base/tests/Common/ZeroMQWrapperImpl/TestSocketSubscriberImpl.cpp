//
// Created by pair on 08/06/18.
//



#include <gtest/gtest.h>

#include <Common/Exceptions/Print.h>

#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/IContextPtr.h>
#include <Common/ZeroMQWrapper/IIPCException.h>

#include <thread>

using Common::ZeroMQWrapper::ISocketSubscriberPtr;

namespace
{
    TEST(TestSocketSubscriberImpl, creation) // NOLINT
    {
        Common::ZeroMQWrapper::IContextPtr context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        EXPECT_NE(socket.get(), nullptr);
    }

    TEST(TestSocketSubscriberImpl, listen) // NOLINT
    {
        Common::ZeroMQWrapper::IContextPtr context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        ASSERT_NE(socket.get(), nullptr);
        socket->listen("inproc://transferTest");
        socket->subscribeTo("FOOBAR");
    }

    class SenderThread
    {
    public:
        explicit SenderThread(Common::ZeroMQWrapper::IContext& context);
        ~SenderThread() {
            m_stopThread = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }
        void start();
    private:
        Common::ZeroMQWrapper::IContext& m_context;
        std::thread m_thread;
        bool m_stopThread;
        void run();
    };

    SenderThread::SenderThread(Common::ZeroMQWrapper::IContext &context)
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
            try
            {
                sender->write({"FOOBAR", "DATA"});
            }
            catch (const Common::ZeroMQWrapper::IIPCException& e)
            {
                PRINT("Failed to send subscription data: "<< e.what());
            }

            std::this_thread::sleep_for( std::chrono::milliseconds(10) );
        }
    }

    TEST(TestSocketSubscriberImpl, comms) // NOLINT
    {
        Common::ZeroMQWrapper::IContextPtr context = Common::ZeroMQWrapper::createContext();
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
