//
// Created by pair on 11/06/18.
//


#include <gtest/gtest.h>

#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/IContextPtr.h>
#include <Common/ZeroMQWrapper/IReadWrite.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <Common/Exceptions/Print.h>
#include <thread>

using namespace Common::ZeroMQWrapper;

namespace
{
    TEST(TestPollerImpl, Creation) // NOLINT
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(),nullptr);
    }

    TEST(TestPollerImpl, CanAddFd) // NOLINT
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(),nullptr);
        int pipefds[2];
        int rc = ::pipe(pipefds);
        ASSERT_EQ(rc,0);
        IHasFDPtr fd1 = poller->addEntry(pipefds[0],Common::ZeroMQWrapper::IPoller::POLLIN);
        ASSERT_EQ(fd1->fd(),pipefds[0]);
        IHasFDPtr fd2 = poller->addEntry(pipefds[1],Common::ZeroMQWrapper::IPoller::POLLOUT);
        ASSERT_EQ(fd2->fd(),pipefds[1]);
        poller.reset();
        ::close(pipefds[0]);
        ::close(pipefds[1]);
    }

    TEST(TestPollerImpl, CanAddSocket) // NOLINT
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(),nullptr);

        IContextPtr context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        ASSERT_NE(socket.get(),nullptr);

        poller->addEntry(*socket, Common::ZeroMQWrapper::IPoller::POLLIN);
    }

    TEST(TestPollerImpl, PollTimeout) // NOLINT
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(),nullptr);

        auto result = poller->poll(10); // 10 ms
        EXPECT_EQ(result.size(),0);
    }

    TEST(TestPollerImpl, REPSocketNotified) // NOLINT
    {
        IPollerPtr poller = Common::ZeroMQWrapper::createPoller();
        ASSERT_NE(poller.get(),nullptr);

        auto context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        auto replier = context->getReplier();
        ASSERT_NE(replier.get(),nullptr);
        auto requester = context->getRequester();
        ASSERT_NE(requester.get(),nullptr);

        replier->listen("inproc://REPSocketNotified");
        requester->connect("inproc://REPSocketNotified");

        using data_t = Common::ZeroMQWrapper::IWritable::data_t;

        data_t input{"FOO","BAR"};
        requester->write(input);

        poller->addEntry(*replier,Common::ZeroMQWrapper::IPoller::POLLIN);
        poller->addEntry(*requester,Common::ZeroMQWrapper::IPoller::POLLIN);

        auto result = poller->poll(10);

        EXPECT_EQ(result.size(),1);
        EXPECT_EQ(result.at(0),replier.get());

        data_t output = replier->read();
        EXPECT_EQ(input,output);

        data_t input2{"RES"};
        replier->write(input2);

        result = poller->poll(10);

        EXPECT_EQ(result.size(),1);
        EXPECT_EQ(result.at(0),requester.get());

        data_t output2 = requester->read();

        EXPECT_EQ(input2,output2);
    }
}