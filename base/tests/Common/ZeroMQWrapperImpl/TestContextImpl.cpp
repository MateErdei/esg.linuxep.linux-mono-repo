/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <Common/ZeroMQWrapper/ISocketRequester.h>

#include <Common/ZeroMQWrapperImpl/ContextImpl.h>



namespace
{
    TEST(TestContextImpl, Creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context
                (new Common::ZeroMQWrapperImpl::ContextImpl());
    }

    TEST(TestContextImpl, Factory) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context,nullptr);
    }

    TEST(TestContextImpl, getRequester) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        auto socket = context->getRequester();
        EXPECT_NE(socket.get(),nullptr);
    }
}
