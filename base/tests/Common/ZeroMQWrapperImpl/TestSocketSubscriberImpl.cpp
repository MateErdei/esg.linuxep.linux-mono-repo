//
// Created by pair on 08/06/18.
//



#include <gtest/gtest.h>

#include <Common/ZeroMQ_wrapper/ISocketSubscriber.h>
#include <Common/ZeroMQ_wrapper/IContext.h>

using Common::ZeroMQ_wrapper::ISocketSubscriberPtr;

namespace
{
    TEST(TestSocketSubscriberImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQ_wrapper::IContext> context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketSubscriberPtr socket = context->getSubscriber();
        EXPECT_NE(socket.get(), nullptr);
    }
}
