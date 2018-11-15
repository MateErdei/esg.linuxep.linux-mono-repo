/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include <Common/ZeroMQWrapper/ISocketRequester.h>

#include <Common/ZeroMQWrapperImpl/ContextImpl.h>
#include <Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h>

using Common::ZeroMQWrapper::ISocketRequesterPtr;

namespace
{
    TEST(TestSocketRequesterImpl, creation) // NOLINT
    {
        auto context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
    }

    TEST(TestSocketRequesterImpl, listen) // NOLINT
    {
        auto context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
        socket->listen("inproc://ListenTest");
    }

    TEST(TestSocketRequesterImpl, connectionTimeout) // NOLINT
    {
        auto context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketRequesterPtr socket = context->getRequester();
        socket->setConnectionTimeout(200);
        socket->connect( "ipc:///tmp/no_one_listening.ipc");
        socket->write({"cmd", "arg"});
        EXPECT_THROW(socket->read(), Common::ZeroMQWrapperImpl::ZeroMQWrapperException); //NOLINT
    }

}
