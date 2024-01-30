// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/ZeroMQWrapperImpl/SocketHolder.h"
#include <gtest/gtest.h>

#include <zmq.h>

namespace
{
    Common::ZeroMQWrapperImpl::ContextHolderSharedPtr createContext()
    {
        return std::make_shared<Common::ZeroMQWrapperImpl::ContextHolder>();
    }

    // cppcheck-suppress syntaxError
    TEST(TestSocketHolder, DefaultCreation)
    {
        Common::ZeroMQWrapperImpl::SocketHolder socket;
        ASSERT_EQ(socket.skt(), nullptr);
    }

    TEST(TestSocketHolder, PointerCreation)
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;

        void* callbackSocket = zmq_socket(holder.ctx(), ZMQ_REP);
        Common::ZeroMQWrapperImpl::SocketHolder socket(callbackSocket);
        ASSERT_EQ(socket.skt(), callbackSocket);
    }

    TEST(TestSocketHolder, TestResetReplacesPointer)
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;

        void* callbackSocket = zmq_socket(holder.ctx(), ZMQ_REP);
        Common::ZeroMQWrapperImpl::SocketHolder socket(callbackSocket);
        ASSERT_EQ(socket.skt(), callbackSocket);

        void* socket2 = zmq_socket(holder.ctx(), ZMQ_REP);
        socket.reset(socket2);
        ASSERT_EQ(socket.skt(), socket2);
    }

    TEST(TestSocketHolder, TestInternalCreation)
    {
        Common::ZeroMQWrapperImpl::ContextHolderSharedPtr holder = createContext();
        Common::ZeroMQWrapperImpl::SocketHolder socket(holder, ZMQ_REP);
        ASSERT_NE(socket.skt(), nullptr);

        void* socket2 = zmq_socket(holder->ctx(), ZMQ_REP);
        socket.reset(socket2);
        ASSERT_EQ(socket.skt(), socket2);
    }
} // namespace
