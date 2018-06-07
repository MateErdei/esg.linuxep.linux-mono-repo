//
// Created by pair on 06/06/18.
//

#include <gtest/gtest.h>
#include <Common/ZeroMQWrapperImpl/ContextImpl.h>



namespace
{
    TEST(TestContextImpl, Creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQ_wrapper::IContext> context
                (new Common::ZeroMQWrapperImpl::ContextImpl());
    }

    TEST(TestContextImpl, Factory) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQ_wrapper::IContext> context = Common::ZeroMQ_wrapper::createContext();
        ASSERT_NE(context,nullptr);
    }
}
