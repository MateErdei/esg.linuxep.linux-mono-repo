/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "modules/Common/ZeroMQWrapperImpl/ContextHolder.h"

#include <gtest/gtest.h>

namespace
{
    TEST(TestContextHolder, Creation) // NOLINT
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;
    }

    // cppcheck-suppress syntaxError
    TEST(TestContextHolder, ContainsPointer) // NOLINT
    {
        Common::ZeroMQWrapperImpl::ContextHolder holder;
        ASSERT_NE(holder.ctx(), nullptr);
    }
} // namespace
