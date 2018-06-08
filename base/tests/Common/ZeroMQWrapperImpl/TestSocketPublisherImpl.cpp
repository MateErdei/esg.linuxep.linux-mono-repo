//
// Created by pair on 07/06/18.
//



#include <gtest/gtest.h>

#include <Common/ZeroMQ_wrapper/ISocketPublisher.h>
#include <Common/ZeroMQ_wrapper/IContext.h>

using Common::ZeroMQ_wrapper::ISocketPublisherPtr;

namespace
{
    TEST(TestSocketPublisherImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQ_wrapper::IContext> context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketPublisherPtr socket = context->getPublisher();
        EXPECT_NE(socket.get(), nullptr);
    }
}
