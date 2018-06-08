//
// Created by pair on 07/06/18.
//


#include <gtest/gtest.h>

#include <Common/ZeroMQWrapper/ISocketRequester.h>

#include <Common/ZeroMQWrapperImpl/ContextImpl.h>

using Common::ZeroMQWrapper::ISocketRequesterPtr;

namespace
{
    TEST(TestSocketRequesterImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
    }

    TEST(TestSocketRequesterImpl, listen) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
        socket->listen("inproc://ListenTest");
    }
}
