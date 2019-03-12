/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Exceptions/Print.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZMQWrapperApiImpl/ContextImpl.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapper/IProxy.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapperImpl/ProxyImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketPublisherImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <thread>

namespace
{
    std::string ToString(Common::ZeroMQWrapper::data_t data)
    {
        if (data.empty())
        {
            return "empty";
        }
        else
        {
            std::stringstream s;
            s << "data =";
            for (auto& entry : data)
            {
                s << " " << entry;
            }
            return s.str();
        }
    }
} // namespace

using namespace Common::ZeroMQWrapper;

class TestProxyImpl : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging; // NOLINT

public:
    void SetUp() override { testing::internal::CaptureStderr(); }

    void TearDown() override
    {
        std::string logMessage = testing::internal::GetCapturedStderr();
        ASSERT_EQ(logMessage, ""); // No Logging from ProxyImpl
    }
};

TEST_F(TestProxyImpl, Creation) // NOLINT
{
    Common::ZMQWrapperApi::IContextSharedPtr sharedContextPtr =
        std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
    IProxyPtr proxy = sharedContextPtr->getProxy("inproc://frontend", "inproc://backend");
    EXPECT_NE(proxy.get(), nullptr);
}

TEST_F(TestProxyImpl, StartStop) // NOLINT
{
    Common::ZMQWrapperApi::IContextSharedPtr sharedContextPtr =
        std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
    IProxyPtr proxy = sharedContextPtr->getProxy("inproc://frontend", "inproc://backend");
    ASSERT_NE(proxy.get(), nullptr);

    proxy->start();
    proxy->stop();
}

namespace
{
    class SenderThread
    {
    public:
        explicit SenderThread(
            Common::ZeroMQWrapper::ISocketPublisher& socket,
            std::string frontendAddress,
            Common::ZeroMQWrapper::ISocketPublisher::data_t message);

        ~SenderThread() { stop(); }

        void start();
        void stop()
        {
            m_stopThread = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

    private:
        Common::ZeroMQWrapper::ISocketPublisher& m_socket;
        std::thread m_thread;
        std::atomic<bool> m_stopThread;
        const std::string m_frontendAddress;
        Common::ZeroMQWrapper::ISocketPublisher::data_t m_message;

        void run();
    };

    SenderThread::SenderThread(
        Common::ZeroMQWrapper::ISocketPublisher& socket,
        std::string frontendAddress,
        Common::ZeroMQWrapper::ISocketPublisher::data_t message) :
        m_socket(socket),
        m_thread(),
        m_stopThread(false),
        m_frontendAddress(std::move(frontendAddress)),
        m_message(std::move(message))
    {
    }

    void SenderThread::start() { m_thread = std::thread(&SenderThread::run, this); }

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
            catch (const Common::ZeroMQWrapper::IIPCException& e)
            {
                PRINT("Failed to send subscription data: " << e.what());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
} // namespace

TEST_F(TestProxyImpl, PassMessage) // NOLINT
{
    // Need to share the context to use inproc addresses
    const std::string frontend = "inproc://frontend";
    const std::string backend = "inproc://backend";
    Common::ZMQWrapperApi::IContextSharedPtr sharedContextPtr =
        std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
    IProxyPtr proxy = sharedContextPtr->getProxy(frontend, backend);
    ASSERT_NE(proxy.get(), nullptr);

    proxy->start();

    auto proxyImpl(dynamic_cast<Common::ZeroMQWrapperImpl::ProxyImpl*>(proxy.get()));
    ASSERT_NE(proxyImpl, nullptr);

    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr contextHolder(proxyImpl->ctx());

    // Directly creating to share the same context
    Common::ZeroMQWrapper::ISocketPublisherPtr publisher(
        new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder));

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket(
        new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder));

    ASSERT_NE(socket.get(), nullptr);
    socket->setTimeout(2000);
    socket->connect(backend);
    socket->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread(*publisher, frontend, { "FOOBAR", "DATA" });
    thread.start();

    auto data = socket->read();

    EXPECT_EQ(data.at(0), "FOOBAR");
    EXPECT_EQ(data.at(1), "DATA");

    socket.reset();

    thread.stop();
    publisher.reset();

    proxy->stop();
    proxy.reset();
}

TEST_F(TestProxyImpl, TwoSubscribers) // NOLINT
{
    // Need to share the context to use inproc addresses
    const std::string frontend = "inproc://frontend";
    const std::string backend = "inproc://backend";
    Common::ZMQWrapperApi::IContextSharedPtr sharedContextPtr =
        std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
    IProxyPtr proxy = sharedContextPtr->getProxy(frontend, backend);
    ASSERT_NE(proxy.get(), nullptr);

    proxy->start();

    auto proxyImpl(dynamic_cast<Common::ZeroMQWrapperImpl::ProxyImpl*>(proxy.get()));
    ASSERT_NE(proxyImpl, nullptr);

    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr contextHolder(proxyImpl->ctx());

    // Directly creating to share the same context
    Common::ZeroMQWrapper::ISocketPublisherPtr publisher(
        new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder));

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket1(
        new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder));
    ASSERT_NE(socket1.get(), nullptr);
    socket1->setTimeout(2000);
    socket1->connect(backend);
    socket1->subscribeTo("FOOBAR");

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket2(
        new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder));
    ASSERT_NE(socket2.get(), nullptr);
    socket2->setTimeout(2000);
    socket2->connect(backend);
    socket2->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread(*publisher, frontend, { "FOOBAR", "DATA" });
    thread.start();

    auto data = socket1->read();

    EXPECT_EQ(data.at(0), "FOOBAR");
    EXPECT_EQ(data.at(1), "DATA");

    socket1.reset();

    data = socket2->read();

    EXPECT_EQ(data.at(0), "FOOBAR");
    EXPECT_EQ(data.at(1), "DATA");

    socket2.reset();

    thread.stop();
    publisher.reset();

    proxy->stop();
    proxy.reset();
}

TEST_F(TestProxyImpl, TwoSenders) // NOLINT
{
    // Need to share the context to use inproc addresses
    const std::string frontend = "inproc://frontend";
    const std::string backend = "inproc://backend";
    Common::ZMQWrapperApi::IContextSharedPtr sharedContextPtr =
        std::make_shared<Common::ZMQWrapperApiImpl::ContextImpl>();
    IProxyPtr proxy = sharedContextPtr->getProxy(frontend, backend);
    ASSERT_NE(proxy.get(), nullptr);

    proxy->start();

    auto proxyImpl(dynamic_cast<Common::ZeroMQWrapperImpl::ProxyImpl*>(proxy.get()));
    ASSERT_NE(proxyImpl, nullptr);

    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr contextHolder(proxyImpl->ctx());

    // Directly creating to share the same context
    Common::ZeroMQWrapper::ISocketPublisherPtr publisher1(
        new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder));

    Common::ZeroMQWrapper::ISocketPublisherPtr publisher2(
        new Common::ZeroMQWrapperImpl::SocketPublisherImpl(contextHolder));

    Common::ZeroMQWrapper::ISocketSubscriberPtr socket(
        new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(contextHolder));

    ASSERT_NE(socket.get(), nullptr);
    socket->setTimeout(2000);
    socket->connect(backend);
    socket->subscribeTo("FOOBAR");

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread1(*publisher1, frontend, { "FOOBAR", "DATA" });
    thread1.start();
    bool thread1Sent = false;

    // Start sender thread - since we need to wait for subscription to propagate
    SenderThread thread2(*publisher2, frontend, { "FOOBAR", "OTHER" });
    thread2.start();
    bool thread2Sent = false;

    int count = 50;

    while (!thread1Sent || !thread2Sent || count > 0)
    {
        auto data = socket->read();
        ASSERT_GT(data.size(), 1) << "Socket->read() returned vector of size: " << ToString(data);
        EXPECT_EQ(data.at(0), "FOOBAR");
        std::string v = data.at(1);
        if (v == "DATA")
        {
            thread1Sent = true;
        }
        else if (v == "OTHER")
        {
            thread2Sent = true;
        }
        else
        {
            EXPECT_TRUE(false) << "Unexpected value of received data. " << ToString(data);
        }
        count--;
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
