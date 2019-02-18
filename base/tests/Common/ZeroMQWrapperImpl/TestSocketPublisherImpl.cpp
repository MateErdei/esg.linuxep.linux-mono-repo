/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <gtest/gtest.h>

using Common::ZeroMQWrapper::ISocketPublisherPtr;

namespace
{
    TEST(TestSocketPublisherImpl, creation) // NOLINT
    {
        auto context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketPublisherPtr socket = context->getPublisher();
        EXPECT_NE(socket.get(), nullptr);
    }
} // namespace
