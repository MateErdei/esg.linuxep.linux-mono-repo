/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ZMQWrapperApiImpl/ContextImpl.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>
#include <gtest/gtest.h>

using Common::ZeroMQWrapper::ISocketRequesterPtr;

namespace
{
    TEST(TestSocketRequesterImpl, creation) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(), nullptr);
    }

    // cppcheck-suppress syntaxError
    TEST(TestSocketRequesterImpl, listen) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(), nullptr);
        socket->listen("inproc://ListenTest");
    }

    TEST(TestSocketRequesterImpl, connectionTimeout) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        socket->setConnectionTimeout(200);
        socket->connect("ipc:///tmp/no_one_listening.ipc");
        socket->write({ "cmd", "arg" });
        EXPECT_THROW(socket->read(), Common::ZeroMQWrapperImpl::ZeroMQWrapperException); // NOLINT
    }

} // namespace
