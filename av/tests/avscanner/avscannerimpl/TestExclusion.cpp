/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/Exclusion.h"

using namespace avscanner::avscannerimpl;

TEST(Exclusion, TestStemTypes) // NOLINT
{
    Exclusion rootExcl("/");
    EXPECT_EQ(rootExcl.type(), STEM);

    Exclusion stemExcl("/multiple/nested/dirs/");
    EXPECT_EQ(stemExcl.type(), STEM);

    Exclusion rootExclWithAsterisk("/*");
    EXPECT_EQ(rootExclWithAsterisk.type(), STEM);
    EXPECT_EQ(rootExclWithAsterisk.path(), "/");

    Exclusion stemExclWithAsterisk("/multiple/nested/dirs/*");
    EXPECT_EQ(stemExclWithAsterisk.type(), STEM);
    EXPECT_EQ(stemExclWithAsterisk.path(), "/multiple/nested/dirs/");
}

TEST(Exclusion, TestFullpathTypes) // NOLINT
{
    Exclusion fullpathExcl("/tmp/foo.txt");
    EXPECT_EQ(fullpathExcl.type(), FULLPATH);
}

TEST(Exclusion, TestGlobTypes) // NOLINT
{
    Exclusion globExclAsterix("/tmp/foo*");
    EXPECT_EQ(globExclAsterix.type(), GLOB);
    EXPECT_TRUE(globExclAsterix.appliesToPath("/tmp/foobar"));
    EXPECT_TRUE(globExclAsterix.appliesToPath("/tmp/foo"));
    EXPECT_FALSE(globExclAsterix.appliesToPath("/tmp/fo"));

    Exclusion globExclQuestionMark("/var/log/syslog.?");
    EXPECT_EQ(globExclQuestionMark.type(), GLOB);
    EXPECT_TRUE(globExclQuestionMark.appliesToPath("/var/log/syslog.1"));
    EXPECT_FALSE(globExclQuestionMark.appliesToPath("/var/log/syslog."));

    Exclusion doubleGlobExcl("/tmp*/foo/*");
    EXPECT_EQ(doubleGlobExcl.type(), GLOB);
    EXPECT_TRUE(doubleGlobExcl.appliesToPath("/tmp/foo/bar"));
    EXPECT_TRUE(doubleGlobExcl.appliesToPath("/tmp/bar/foo/"));
    EXPECT_FALSE(doubleGlobExcl.appliesToPath("/home/dev/tmp/bar/foo/"));

    Exclusion regexMetaCharExcl("/tmp/regex[^\\\\]+$filename*");
    EXPECT_EQ(regexMetaCharExcl.type(), GLOB);
    EXPECT_TRUE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\]+$filename"));
    EXPECT_TRUE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\]+$filename.txt"));
    EXPECT_FALSE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\+$filename.txt"));
}

TEST(Exclusion, TestSuffixTypes) // NOLINT
{
    Exclusion suffixExcl("foo.txt");
    EXPECT_EQ(suffixExcl.type(), FILENAME);
    EXPECT_EQ(suffixExcl.path(), "/foo.txt");
}

TEST(Exclusion, TestInvalidTypes) // NOLINT
{
    Exclusion invalidExcl("");
    EXPECT_EQ(invalidExcl.type(), INVALID);
}