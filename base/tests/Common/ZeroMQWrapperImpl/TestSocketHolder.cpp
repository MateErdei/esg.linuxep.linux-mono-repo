//
// Created by pair on 06/06/18.
//


#include <gtest/gtest.h>
#include <Common/ZeroMQWrapperImpl/SocketHolder.h>

namespace
{
    TEST(TestSocketHolder, DefaultCreation) // NOLINT
    {
        Common::ZeroMQWrapperImpl::SocketHolder socket;
        ASSERT_EQ(socket.skt(),nullptr);
    }

    TEST(TestSocketHolder, PointerCreation) // NOLINT
    {
        int foo = 10;
        Common::ZeroMQWrapperImpl::SocketHolder socket(&foo);
        ASSERT_EQ(socket.skt(),&foo);
    }
}
