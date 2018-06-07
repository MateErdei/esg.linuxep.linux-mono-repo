//
// Created by pair on 07/06/18.
//


#include <gtest/gtest.h>

#include <Common/ZeroMQ_wrapper/ISocketRequester.h>

#include <Common/ZeroMQWrapperImpl/ContextImpl.h>

using Common::ZeroMQ_wrapper::ISocketRequesterPtr;

namespace
{
    TEST(TestSocketRequesterImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQ_wrapper::IContext> context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
    }

    TEST(TestSocketRequesterImpl, listen) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQ_wrapper::IContext> context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
        socket->listen("inproc://ListenTest");
    }
}
