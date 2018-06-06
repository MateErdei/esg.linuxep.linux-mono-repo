//
// Created by pair on 06/06/18.
//

#include "gtest/gtest.h"
#include "../../../modules/Common/ZeroMQWrapperImpl/ContextHolder.h"

namespace
{
    TEST(TestContextHolder, Creation) // NOLINT
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;
    }

    TEST(TestContextHolder, ContainsPointer) // NOLINT
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;
        ASSERT_NE(holder.ctx(),nullptr);
    }
}
