/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "Common/ZeroMQWrapperImpl/ContextHolder.h"

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
