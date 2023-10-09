// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <Common/Threads/NotifyPipe.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapper/IReadWrite.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapperImpl/SocketImpl.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>
#include <zmq.h>

using namespace Common::ZeroMQWrapper;

namespace
{
    TEST(TestPollerImpl, Creation)
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(), nullptr);
    }

    // cppcheck-suppress syntaxError
    TEST(TestPollerImpl, CanAddFd)
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(), nullptr);
        int pipefds[2];
        int rc = ::pipe(pipefds);
        ASSERT_EQ(rc, 0);
        IHasFDPtr fd1 = poller->addEntry(pipefds[0], Common::ZeroMQWrapper::IPoller::POLLIN);
        ASSERT_EQ(fd1->fd(), pipefds[0]);
        IHasFDPtr fd2 = poller->addEntry(pipefds[1], Common::ZeroMQWrapper::IPoller::POLLOUT);
        ASSERT_EQ(fd2->fd(), pipefds[1]);
        poller.reset();
        ::close(pipefds[0]);
        ::close(pipefds[1]);
    }

    TEST(TestPollerImpl, CanAddSocket)
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(), nullptr);

        Common::ZMQWrapperApi::IContextSharedPtr context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        ASSERT_NE(socket.get(), nullptr);

        poller->addEntry(*socket, Common::ZeroMQWrapper::IPoller::POLLIN);
    }

    TEST(TestPollerImpl, PollTimeout)
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(), nullptr);

        auto result = poller->poll(Common::ZeroMQWrapper::ms(10)); // 10 ms
        EXPECT_EQ(result.size(), 0);
    }

    TEST(TestPollerImpl, REPSocketNotified)
    {
        using Common::ZeroMQWrapper::ms;

        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(), nullptr);

        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        auto replier = context->getReplier();
        ASSERT_NE(replier.get(), nullptr);
        auto requester = context->getRequester();
        ASSERT_NE(requester.get(), nullptr);

        replier->listen("inproc://REPSocketNotified");
        requester->connect("inproc://REPSocketNotified");

        using data_t = Common::ZeroMQWrapper::IWritable::data_t;

        data_t input{ "FOO", "BAR" };
        requester->write(input);

        poller->addEntry(*replier, Common::ZeroMQWrapper::IPoller::POLLIN);
        poller->addEntry(*requester, Common::ZeroMQWrapper::IPoller::POLLIN);

        auto result = poller->poll(ms(10));

        EXPECT_EQ(result.size(), 1);
        EXPECT_EQ(result.at(0), replier.get());

        data_t output = replier->read();
        EXPECT_EQ(input, output);

        data_t input2{ "RES" };
        replier->write(input2);

        result = poller->poll(ms(10));

        EXPECT_EQ(result.size(), 1);
        EXPECT_EQ(result.at(0), requester.get());

        data_t output2 = requester->read();

        EXPECT_EQ(input2, output2);
    }

    TEST(TestPollerImpl, PollerShouldThrowExceptionIfFileDescriptorCloses)
    {
        using Common::Threads::NotifyPipe;
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        std::unique_ptr<NotifyPipe> notifyPipe(new NotifyPipe());
        NotifyPipe notifyPipe1;
        auto pipeFD = poller->addEntry(notifyPipe->readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);
        auto pipe2FD = poller->addEntry(notifyPipe1.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

        auto notifyFuture = std::async(std::launch::async, [&notifyPipe]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            notifyPipe->notify();
        });

        auto result = poller->poll(Common::ZeroMQWrapper::ms(200));
        EXPECT_EQ(result.size(), 1);
        EXPECT_EQ(result.at(0)->fd(), notifyPipe->readFd());
        EXPECT_TRUE(notifyPipe->notified());

        // timeout
        result = poller->poll(Common::ZeroMQWrapper::ms(10));
        EXPECT_EQ(result.size(), 0);

        // close notifyPipe
        notifyPipe.reset();
        EXPECT_THROW(
            poller->poll(Common::ZeroMQWrapper::ms(2000)),
            Common::ZeroMQWrapperImpl::ZeroMQPollerException);
    }

    class SocketReleaser
    {
        Common::ZeroMQWrapperImpl::SocketHolder& m_socket;

    public:
        explicit SocketReleaser(Common::ZeroMQWrapperImpl::SocketHolder& socket) : m_socket(socket) {}

        ~SocketReleaser() { release(); }

        void* release() { return m_socket.release(); }
    };

    TEST(TestPollerImpl, PollerShouldThrowExceptionIfUnderlingSocketCloses)
    {
        constexpr int POLL_MS = 2000;
        using Common::Threads::NotifyPipe;
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

        auto context = Common::ZMQWrapperApi::createContext();
        auto replier = context->getReplier();
        auto requester = context->getRequester();
        replier->listen("inproc://PollerShouldThrowExceptionIfUnderlingSocketCloses");
        requester->setTimeout(200);
        requester->connect("inproc://PollerShouldThrowExceptionIfUnderlingSocketCloses");
        poller->addEntry(*replier, Common::ZeroMQWrapper::IPoller::POLLIN);

        auto notifyFuture = std::async(std::launch::async, [&requester]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            requester->write({ "hello1" });
        });
        auto result = poller->poll(Common::ZeroMQWrapper::ms(POLL_MS));
        notifyFuture.get();
        EXPECT_EQ(result.size(), 1);

        EXPECT_EQ(result.at(0)->fd(), replier->fd());
        EXPECT_EQ(replier->read(), std::vector<std::string>{ "hello1" });
        replier->write({ "world" });
        EXPECT_EQ(requester->read(), std::vector<std::string>{ "world" });

        auto socket = dynamic_cast<Common::ZeroMQWrapperImpl::SocketImpl*>(replier.get());
        ASSERT_TRUE(socket != nullptr);
        auto zmqsocket = socket->skt();
        ASSERT_EQ(zmq_close(zmqsocket), 0);
        requester->write({ "another request" });
        EXPECT_THROW(
            poller->poll(Common::ZeroMQWrapper::ms(POLL_MS)),
            Common::ZeroMQWrapperImpl::ZeroMQPollerException);

        poller.reset();
        socket->socketHolder().release(); // Socket already closed
        replier.reset();
        requester.reset();
        context.reset();
    }

    TEST(TestPollerImpl, AddToPollerAnInvalidFileDescriptorShouldThrowException)
    {
        using Common::Threads::NotifyPipe;
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        std::unique_ptr<NotifyPipe> notifyPipe(new NotifyPipe());
        NotifyPipe notifyPipe1;
        int invalidFD = notifyPipe->readFd();
        notifyPipe.reset();
        auto pipeFD = poller->addEntry(invalidFD, Common::ZeroMQWrapper::IPoller::POLLIN);
        auto pipe2FD = poller->addEntry(notifyPipe1.readFd(), Common::ZeroMQWrapper::IPoller::POLLIN);

        EXPECT_THROW(
            poller->poll(Common::ZeroMQWrapper::ms(2000)),
            Common::ZeroMQWrapperImpl::ZeroMQPollerException);
    }

    TEST(TestPollerImpl, AddToPollerAnInvalidSocketShouldThrowException)
    {
        using Common::Threads::NotifyPipe;
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

        auto context = Common::ZMQWrapperApi::createContext();
        auto replier = context->getReplier();
        auto requester = context->getRequester();
        replier->listen("inproc://REPSocketNotified");
        requester->setTimeout(200);
        requester->connect("inproc://REPSocketNotified");

        auto socket = dynamic_cast<Common::ZeroMQWrapperImpl::SocketImpl*>(replier.get());
        ASSERT_TRUE(socket != nullptr);
        auto zmqsocket = socket->skt();
        ASSERT_EQ(zmq_close(zmqsocket), 0);

        poller->addEntry(*replier, Common::ZeroMQWrapper::IPoller::POLLIN);

        EXPECT_THROW(
            poller->poll(Common::ZeroMQWrapper::ms(2000)),
            Common::ZeroMQWrapperImpl::ZeroMQPollerException);

        socket->socketHolder().release(); // Socket already closed
    }

    TEST(TestPollerImpl, ThePollerShouldReturnThatDataIsAvailableWhileItIsAvailable)
    {
        int filedes[2];
        ASSERT_EQ(pipe(filedes), 0);
        int readFd = filedes[0];
        int writeFd = filedes[1];
        std::string writeCnt = "1";
        EXPECT_EQ(::write(writeFd, writeCnt.c_str(), writeCnt.size()), writeCnt.size());
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        auto pipeFD = poller->addEntry(readFd, Common::ZeroMQWrapper::IPoller::POLLIN);
        auto res = poller->poll(std::chrono::milliseconds(200));
        EXPECT_EQ(res.size(), 1);
        EXPECT_EQ(::close(writeFd), 0);
        // closed but with data available should return the file descriptor from where data can be read.
        res = poller->poll(std::chrono::milliseconds(200));
        EXPECT_EQ(res.size(), 1);

        // read all the information from the buffer.
        std::array<unsigned char, 10> buffer{};
        EXPECT_EQ(::read(readFd, buffer.data(), writeCnt.size()), writeCnt.size());

        EXPECT_THROW(
            poller->poll(Common::ZeroMQWrapper::ms(2000)),
            Common::ZeroMQWrapperImpl::ZeroMQPollerException);

        EXPECT_EQ(::close(readFd), 0);
        //
    }

} // namespace
