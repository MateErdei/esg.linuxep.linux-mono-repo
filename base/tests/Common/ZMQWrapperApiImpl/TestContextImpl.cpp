/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "modules/Common/ZMQWrapperApiImpl/ContextImpl.h"
#include "modules/Common/ZeroMQWrapper/ISocketRequester.h"
#include <gtest/gtest.h>

namespace
{
    TEST(TestContextImpl, Creation) // NOLINT
    {
        Common::ZMQWrapperApi::IContextSharedPtr context(new Common::ZMQWrapperApiImpl::ContextImpl());
    }

    // cppcheck-suppress syntaxError
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
