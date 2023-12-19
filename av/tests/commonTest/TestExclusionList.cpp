// Copyright 2023 Sophos Limited. All rights reserved.

#include "common/ExclusionList.h"

#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>

using namespace common;

namespace
{
    class TestExclusionList : public LogInitializedTests
    {
    };
}

TEST_F(TestExclusionList, default_is_empty)
{
    ExclusionList x;
    EXPECT_TRUE(x.empty());
}

TEST_F(TestExclusionList, empty_is_empty)
{
    ExclusionList x{{}};
    EXPECT_TRUE(x.empty());
}

TEST_F(TestExclusionList, one_string_is_not_empty)
{
    ExclusionList x{{"/a/b/c"}};
    EXPECT_FALSE(x.empty());
}

TEST_F(TestExclusionList, fixed_string_match)
{
    ExclusionList x{{"/a/b/c"}};
    CachedPath p{std::string{"/a/b/c"}};
    EXPECT_TRUE(x.appliesToPath(p, false, true));
}

TEST_F(TestExclusionList, fixed_string_no_match)
{
    ExclusionList x{{"/a/b/c"}};
    CachedPath p{std::string{"/a/b/d"}};
    EXPECT_FALSE(x.appliesToPath(p, false, true));
}

TEST_F(TestExclusionList, sorts_exclusions)
{
    ExclusionList x{{"/mnt*foo/","/uk-filer5/"}};
    ASSERT_EQ(x.size(), 2);
    EXPECT_LT(x.at(0), x.at(1));
    EXPECT_EQ(x.at(0).type(), ExclusionType::STEM);
    EXPECT_EQ(x.at(0).displayPath(), "/uk-filer5/");
    EXPECT_EQ(x[0].displayPath(), "/uk-filer5/");
}

TEST_F(TestExclusionList, multiple_globs_all_match)
{
    ExclusionList x{{"/mnt*foo/","/a/*/b", "/c*d"}};
    EXPECT_GT(x.size(), 0);
    EXPECT_TRUE(x.appliesToPath("/mntabcfoo/something", false, true));
    EXPECT_TRUE(x.appliesToPath("/a/foo/b", false, true));
    EXPECT_TRUE(x.appliesToPath("/cfood", false, true));
    EXPECT_FALSE(x.appliesToPath("/nothing", false, true));
}
