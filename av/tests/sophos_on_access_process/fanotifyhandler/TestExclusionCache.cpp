// Copyright 2023 Sophos All rights reserved.

#include "sophos_on_access_process/fanotifyhandler/ExclusionCache.h"

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

namespace
{
    class TestExclusionCache : public FanotifyHandlerMemoryAppenderUsingTests
    {
    };
}

TEST_F(TestExclusionCache, construction)
{
    EXPECT_NO_THROW(sophos_on_access_process::fanotifyhandler::ExclusionCache cache);
}

TEST_F(TestExclusionCache, initially_no_exclusions)
{
    sophos_on_access_process::fanotifyhandler::ExclusionCache cache;
    EXPECT_EQ(cache.m_exclusions.size(), 0);
}

TEST_F(TestExclusionCache, canSetExclusions)
{
    sophos_on_access_process::fanotifyhandler::ExclusionCache cache;
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back("/excluded/");
    EXPECT_NO_THROW(cache.setExclusions(exclusions));
    ASSERT_EQ(cache.m_exclusions.size(), 1);
    EXPECT_EQ(cache.m_exclusions[0].displayPath(), "/excluded/");
}
