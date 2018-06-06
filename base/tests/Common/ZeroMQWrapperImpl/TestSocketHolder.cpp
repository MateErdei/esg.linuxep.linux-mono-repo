//
// Created by pair on 06/06/18.
//


#include <gtest/gtest.h>
#include <Common/ZeroMQWrapperImpl/SocketHolder.h>
#include <Common/ZeroMQWrapperImpl/ContextHolder.h>
#include <zmq.h>

namespace
{
    TEST(TestSocketHolder, DefaultCreation) // NOLINT
    {
        Common::ZeroMQWrapperImpl::SocketHolder socket;
        ASSERT_EQ(socket.skt(),nullptr);
    }

    TEST(TestSocketHolder, PointerCreation) // NOLINT
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;

        void* callbackSocket = zmq_socket(holder.ctx(), ZMQ_REP);
        Common::ZeroMQWrapperImpl::SocketHolder socket(callbackSocket);
        ASSERT_EQ(socket.skt(),callbackSocket);
    }

    TEST(TestSocketHolder, TestResetReplacesPointer) // NOLINT
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;

        void* callbackSocket = zmq_socket(holder.ctx(), ZMQ_REP);
        Common::ZeroMQWrapperImpl::SocketHolder socket(callbackSocket);
        ASSERT_EQ(socket.skt(),callbackSocket);

        void* socket2 = zmq_socket(holder.ctx(), ZMQ_REP);
        socket.reset(socket2);
        ASSERT_EQ(socket.skt(),socket2);
    }
}
