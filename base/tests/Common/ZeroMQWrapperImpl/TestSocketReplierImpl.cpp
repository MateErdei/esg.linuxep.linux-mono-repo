//
// Created by pair on 07/06/18.
//


#include <gtest/gtest.h>

#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>

#include <Common/ZeroMQWrapperImpl/ContextImpl.h>
#include <sys/socket.h>

using Common::ZeroMQWrapper::ISocketReplierPtr;

namespace
{
    TEST(SocketReplierImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketReplierPtr socket = context->getReplier();
        EXPECT_NE(socket.get(),nullptr);
    }

    TEST(SocketReplierImpl, listen) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        auto socket = context->getReplier();
        EXPECT_NE(socket.get(),nullptr);
        socket->listen("inproc://ListenTest");
    }

    TEST(SocketReplierImpl, transfer) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        auto replier = context->getReplier();
        auto requester = context->getRequester();
        EXPECT_NE(replier.get(),nullptr);
        replier->listen("inproc://transferTest");
        requester->connect("inproc://transferTest");

        using data_t = std::vector<std::string>;

        data_t input{"FOO","BAR"};
        requester->write(input);

        data_t output = replier->read();

        EXPECT_EQ(input,output);

        data_t input2{"RES"};
        replier->write(input2);

        data_t output2 = requester->read();

        EXPECT_EQ(input2,output2);
    }
}
