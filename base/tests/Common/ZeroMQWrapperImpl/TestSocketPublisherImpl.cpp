/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "modules/Common/ZMQWrapperApi/IContext.h"
#include "modules/Common/ZeroMQWrapper/ISocketPublisher.h"
#include <gtest/gtest.h>

using Common::ZeroMQWrapper::ISocketPublisherPtr;

namespace
{
    TEST(TestSocketPublisherImpl, creation) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketPublisherPtr socket = context->getPublisher();
        EXPECT_NE(socket.get(), nullptr);
    }
} // namespace
