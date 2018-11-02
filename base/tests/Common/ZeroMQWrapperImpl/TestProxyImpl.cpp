///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include <Common/Exceptions/Print.h>

#include <Common/ZeroMQWrapper/IProxy.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/IContext.h>

#include <thread>
#include <Common/ZeroMQWrapperImpl/ProxyImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketPublisherImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h>

using namespace Common::ZeroMQWrapper;

TEST(TestProxyImpl, Creation) // NOLINT
{
    IProxyPtr proxy = createProxy("inproc://frontend","inproc://backend");
    EXPECT_NE(proxy.get(),nullptr);
}

TEST(TestProxyImpl, StartStop) // NOLINT
{
    IProxyPtr proxy = createProxy("inproc://frontend","inproc://backend");
    ASSERT_NE(proxy.get(),nullptr);

    proxy->start();
    proxy->stop();
}

namespace
{
    class SenderThread
    {
    public:
        explicit SenderThread(Common::ZeroMQWrapper::ISocketPublisher& socket, std::string frontendAddress, Common::ZeroMQWrapper::ISocketPublisher::data_t message);

        ~SenderThread()
        {
            m_stopThread = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        void start();
        void stop()
        {
            m_stopThread = true;
        }

    private:
        Common::ZeroMQWrapper::ISocketPublisher& m_socket;
        std::thread m_thread;
        bool m_stopThread;
        const std::string m_frontendAddress;
        Common::ZeroMQWrapper::ISocketPublisher::data_t m_message;

        void run();
    };

    SenderThread::SenderThread(Common::ZeroMQWrapper::ISocketPublisher& socket, std::string frontendAddress, Common::ZeroMQWrapper::ISocketPublisher::data_t message)
            : m_socket(socket),
              m_thread(),
              m_stopThread(false),
              m_frontendAddress(std::move(frontendAddress)),
              m_message(std::move(message))
    {
    }

    void SenderThread::start()
    {
        m_thread = std::thread(&SenderThread::run, this);
    }

    void SenderThread::run()
    {
        auto sender = &m_socket;

        sender->setTimeout(2000);

        // Start sender thread - since we need to wait for subscription to propagate
        sender->connect(m_frontendAddress);

        while (!m_stopThread)
        {
            try
            {
                sender->write(m_message);
            }
            catch (const Common::ZeroMQWrapper::IIPCException &e)
            {
                PRINT("Failed to send subscription data: " << e.what());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

TEST(TestProxyImpl, PassMessage) // NOLINT
{
    // Need to share the context to use inproc addresses

    const std::string frontend="inproc://frontend";
    const std::string backend="inproc://backend";
    IProxyPtr proxy = createProxy(frontend,backend);
    ASSERT_NE(proxy.get(),nullptr);

    proxy->start();

    auto proxyImpl(dynamic_cast<Common::ZeroMQWrapperImpl::ProxyImpl*>(proxy.get()));
    ASSERT_NE(proxyImpl,nullptr);

    Common::ZeroMQWrapperImpl::ContextHolder& contextHolder(proxyImpl->ctx());

    // Directly creating to share the same context
    Common::ZeroMQWrapper::ISocketPublisherPtr publisher(
            new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder)
        );

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket(
            new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder)
            );

    ASSERT_NE(socket.get(), nullptr);
    socket->setTimeout(2000);
    socket->connect(backend);
    socket->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread(*publisher,frontend,{"FOOBAR", "DATA"});
    thread.start();

    auto data = socket->read();

    EXPECT_EQ(data.at(0),"FOOBAR");
    EXPECT_EQ(data.at(1),"DATA");

    socket.reset();

    thread.stop();
    publisher.reset();

    proxy->stop();
    proxy.reset();
}


TEST(TestProxyImpl, 2subscribers) // NOLINT
{

    // Need to share the context to use inproc addresses

    const std::string frontend="inproc://frontend";
    const std::string backend="inproc://backend";
    IProxyPtr proxy = createProxy(frontend,backend);
    ASSERT_NE(proxy.get(),nullptr);

    proxy->start();

    auto proxyImpl(dynamic_cast<Common::ZeroMQWrapperImpl::ProxyImpl*>(proxy.get()));
    ASSERT_NE(proxyImpl,nullptr);

    Common::ZeroMQWrapperImpl::ContextHolder& contextHolder(proxyImpl->ctx());

    // Directly creating to share the same context
    Common::ZeroMQWrapper::ISocketPublisherPtr publisher(
            new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder)
    );

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket1(
            new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder)
    );
    ASSERT_NE(socket1.get(), nullptr);
    socket1->setTimeout(2000);
    socket1->connect(backend);
    socket1->subscribeTo("FOOBAR");


    Common::ZeroMQWrapper::ISocketSubscriberPtr socket2(
            new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder)
    );
    ASSERT_NE(socket2.get(), nullptr);
    socket2->setTimeout(2000);
    socket2->connect(backend);
    socket2->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread(*publisher,frontend,{"FOOBAR", "DATA"});
    thread.start();

    auto data = socket1->read();

    EXPECT_EQ(data.at(0),"FOOBAR");
    EXPECT_EQ(data.at(1),"DATA");

    socket1.reset();

    data = socket2->read();

    EXPECT_EQ(data.at(0),"FOOBAR");
    EXPECT_EQ(data.at(1),"DATA");

    socket2.reset();

    thread.stop();
    publisher.reset();

    proxy->stop();
    proxy.reset();
}

TEST(TestProxyImpl, 2Senders) // NOLINT
{
    // Need to share the context to use inproc addresses
    const std::string frontend="inproc://frontend";
    const std::string backend="inproc://backend";
    IProxyPtr proxy = createProxy(frontend,backend);
    ASSERT_NE(proxy.get(),nullptr);

    proxy->start();

    auto proxyImpl(dynamic_cast<Common::ZeroMQWrapperImpl::ProxyImpl*>(proxy.get()));
    ASSERT_NE(proxyImpl,nullptr);

    Common::ZeroMQWrapperImpl::ContextHolder& contextHolder(proxyImpl->ctx());

    // Directly creating to share the same context
    Common::ZeroMQWrapper::ISocketPublisherPtr publisher1(
            new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder)
    );

    Common::ZeroMQWrapper::ISocketPublisherPtr publisher2(
            new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder)
    );

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket(
            new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder)
    );

    ASSERT_NE(socket.get(), nullptr);
    socket->setTimeout(2000);
    socket->connect(backend);
    socket->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread1(*publisher1,frontend,{"FOOBAR", "DATA"});
    thread1.start();
    bool thread1Sent = false;

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread2(*publisher2,frontend,{"FOOBAR", "OTHER"});
    thread2.start();
    bool thread2Sent = false;

    int count = 50;

    while (!thread1Sent || !thread2Sent || count == 0)
    {
        auto data = socket->read();

        EXPECT_EQ(data.at(0), "FOOBAR");
        std::string v = data.at(1);
        if (v == "DATA")
        {
            thread1Sent = true;
        } else if (v == "OTHER")
        {
            thread2Sent = true;
        } else
        {
            PRINT("Unexpected value of data.at(1): "<<v<<" @ "<<data.size()<<" @ "<<count);
            if (data.size() == 3 )
            {
                PRINT("Got a vector of "<< data.at(0) <<", "<< data.at(1) << ", " << data.at(2));
            }
            else
            {
                EXPECT_TRUE(v == "DATA" || v == "OTHER");
            }
        }
        count -= 1;
    }

    socket.reset();

    thread1.stop();
    thread2.stop();
    publisher1.reset();
    publisher2.reset();

    proxy->stop();
    proxy.reset();

    EXPECT_TRUE(thread1Sent);
    EXPECT_TRUE(thread2Sent);
}
