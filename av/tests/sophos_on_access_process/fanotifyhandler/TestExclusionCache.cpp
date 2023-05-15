// Copyright 2023 Sophos All rights reserved.

#define TEST_PUBLIC public

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

TEST_F(TestExclusionCache, canMatchExclusions)
{
    sophos_on_access_process::fanotifyhandler::ExclusionCache cache;
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back("/excluded/");
    EXPECT_NO_THROW(cache.setExclusions(exclusions));
    EXPECT_TRUE(cache.checkExclusions("/excluded/foo"));
    EXPECT_FALSE(cache.checkExclusions("/included/foo"));
}

TEST_F(TestExclusionCache, logsRelativeGlobs)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    sophos_on_access_process::fanotifyhandler::ExclusionCache cache;
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back("do*er/");
    EXPECT_NO_THROW(cache.setExclusions(exclusions));
    EXPECT_TRUE(appenderContains("Updating on-access exclusions with: [\"*/do*er/*\"]"));
}

namespace
{
    class CountingExclusionCache : public sophos_on_access_process::fanotifyhandler::ExclusionCache
    {
    public:
        explicit CountingExclusionCache(const std::string& exclusion)
        {
            std::vector<common::Exclusion> exclusions;
            exclusions.emplace_back(exclusion);
            setExclusions(exclusions);
        }
        bool checkExclusionsUncached(const std::string& filePath) const override;
        mutable int count_{0};

        void setCacheLifetime(std::chrono::milliseconds lifetime)
        {
            cache_lifetime_ = lifetime;
        }
    };

    bool CountingExclusionCache::checkExclusionsUncached(const std::string& filePath) const
    {
        count_++;
        return ExclusionCache::checkExclusionsUncached(filePath);
    }
}

TEST_F(TestExclusionCache, cacheExclusions)
{
    CountingExclusionCache cache{"/excluded/"};
    EXPECT_TRUE(cache.checkExclusions("/excluded/foo"));
    EXPECT_TRUE(cache.checkExclusions("/excluded/foo"));
    EXPECT_EQ(cache.count_, 1);
}

TEST_F(TestExclusionCache, cacheExpires)
{
    CountingExclusionCache cache{"/excluded/"};
    cache.setCacheLifetime(std::chrono::milliseconds{1});
    EXPECT_TRUE(cache.checkExclusions("/excluded/foo"));
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
    EXPECT_TRUE(cache.checkExclusions("/excluded/foo"));
    EXPECT_EQ(cache.count_, 2);
}
