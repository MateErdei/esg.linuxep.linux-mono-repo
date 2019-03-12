/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ZMQWrapperApiImpl/ContextImpl.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <gtest/gtest.h>

namespace
{
    TEST(TestContextImpl, Creation) // NOLINT
    {
        Common::ZMQWrapperApi::IContextSharedPtr context(new Common::ZMQWrapperApiImpl::ContextImpl());
    }

    TEST(TestContextImpl, Factory) // NOLINT
    {
        Common::ZMQWrapperApi::IContextSharedPtr context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context, nullptr);
    }

    TEST(TestContextImpl, getRequester) // NOLINT
    {
        Common::ZMQWrapperApi::IContextSharedPtr context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        auto socket = context->getRequester();
        EXPECT_NE(socket.get(), nullptr);
    }
} // namespace
