/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/sophos_filesystem.h"

#include <common/PathUtils.h>
#include <gtest/gtest.h>

namespace fs = sophos_filesystem;
using namespace common;

TEST(TestPathUtils, TestAppendForwardSlashToPath) // NOLINT
{
    fs::path pathWithNoSlash("/path/with/no/slash");
    fs::path pathWithSlash("/path/with/slash/");
    PathUtils::appendForwardSlashToPath(pathWithNoSlash);
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithSlash), pathWithSlash.string());
}

TEST(TestPathUtils, TestAppendForwardSlashToPathWithEscapeCharacters) // NOLINT
{
    fs::path pathWithNoSlash("\1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \a \b \t \n \v \f \r");
    fs::path pathWithSlash("\1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \a \b \t \n \v \f \r/");
    PathUtils::appendForwardSlashToPath(pathWithNoSlash);
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithNoSlash), pathWithNoSlash.string() + "/");
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithSlash), pathWithSlash.string());
}

TEST(TestPathUtils, TestAppendForwardSlashToPathExtendedUnicodeCharacters) // NOLINT
{
    fs::path pathWithNoSlash("大夨天太夫");
    fs::path pathWithSlash("大夨天太夫/");
    PathUtils::appendForwardSlashToPath(pathWithNoSlash);
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithNoSlash), pathWithNoSlash.string() + "/");
    EXPECT_EQ(PathUtils::appendForwardSlashToPath(pathWithSlash), pathWithSlash.string());
}

TEST(TestPathUtils, TestLexicalNormalisation) // NOLINT
{
    EXPECT_EQ(PathUtils::lexicallyNormal(".."), "");
    EXPECT_EQ(PathUtils::lexicallyNormal("/.."), "/");
    EXPECT_EQ(PathUtils::lexicallyNormal("/./a"), "/a");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/.."), "/");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/../b"), "/b");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/..b"), "/a/..b");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/..b/"), "/a/..b/");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/./b"), "/a/b");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/.b"), "/a/.b");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/.b/"), "/a/.b/");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/b/../b/./c"), "/a/b/c");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/b/../b/./c/"), "/a/b/c/");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/b/../b/./c/."), "/a/b/c/");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/b/../b/c"), "/a/b/c");
    EXPECT_EQ(PathUtils::lexicallyNormal("/a/b/./c"), "/a/b/c");
    EXPECT_EQ(PathUtils::lexicallyNormal("a//b"), "a/b");
    EXPECT_EQ(PathUtils::lexicallyNormal("a/b/../b/./c/"), "a/b/c/");
    EXPECT_EQ(PathUtils::lexicallyNormal("a/b/../b/./c/."), "a/b/c/");
}

TEST(TestPathUtils, TestRemovingTrailingDotFromPath) // NOLINT
{
    EXPECT_EQ(PathUtils::removeTrailingDotsFromPath("/a/b/c/"), "/a/b/c/");
    EXPECT_EQ(PathUtils::removeTrailingDotsFromPath("/a/b/c/."), "/a/b/c/");
}

TEST(TestPathUtils, TestIsNonNormalisedPath) // NOLINT
{
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("a//b"));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("."));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath(".."));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("/./a"));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("/../a"));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("/a/."));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("/a/.."));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("./a"));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("../a"));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("/a/./b"));
    EXPECT_TRUE(PathUtils::isNonNormalisedPath("/a/../b"));

    EXPECT_FALSE(PathUtils::isNonNormalisedPath(".foo"));
    EXPECT_FALSE(PathUtils::isNonNormalisedPath("bar/.foo"));
    EXPECT_FALSE(PathUtils::isNonNormalisedPath("..foo"));
    EXPECT_FALSE(PathUtils::isNonNormalisedPath("bar/..foo"));
    EXPECT_FALSE(PathUtils::isNonNormalisedPath("..."));
    EXPECT_FALSE(PathUtils::isNonNormalisedPath("bar/..."));
}