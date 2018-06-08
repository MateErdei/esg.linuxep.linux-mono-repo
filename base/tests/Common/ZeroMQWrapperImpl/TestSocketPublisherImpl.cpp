//
// Created by pair on 07/06/18.
//



#include <gtest/gtest.h>

#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/IContext.h>

using Common::ZeroMQWrapper::ISocketPublisherPtr;

namespace
{
    TEST(TestSocketPublisherImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketPublisherPtr socket = context->getPublisher();
        EXPECT_NE(socket.get(), nullptr);
    }
}
