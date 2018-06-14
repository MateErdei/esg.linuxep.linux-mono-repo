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
        explicit SenderThread(Common::ZeroMQWrapper::ISocketPublisher& socket, std::string frontendAddress);

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

        void run();
    };

    SenderThread::SenderThread(Common::ZeroMQWrapper::ISocketPublisher& socket, std::string frontendAddress)
            : m_socket(socket),
              m_thread(),
              m_stopThread(false),
              m_frontendAddress(std::move(frontendAddress))
    {
    }

    void SenderThread::start()
    {
        m_thread = std::thread(&SenderThread::run, this);
    }

    void SenderThread::run()
    {
        auto sender = &m_socket;

        sender->setTimeout(1000);

        // Start sender thread - since we need to wait for subscription to propagate
        sender->connect(m_frontendAddress);

        while (!m_stopThread)
        {
            try
            {
                sender->write({"FOOBAR", "DATA"});
            }
            catch (const Common::ZeroMQWrapper::IIPCException &e)
            {
                PRINT("Failed to send subscription data: " << e.what());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    socket->setTimeout(1000);
    socket->connect(backend);
    socket->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread(*publisher,frontend);
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