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
    Exclusion globExclAsteriskEnd("/tmp/foo*");
    EXPECT_EQ(globExclAsteriskEnd.type(), GLOB);
    EXPECT_TRUE(globExclAsteriskEnd.appliesToPath("/tmp/foobar"));
    EXPECT_TRUE(globExclAsteriskEnd.appliesToPath("/tmp/foo"));
    EXPECT_FALSE(globExclAsteriskEnd.appliesToPath("/tmp/fo"));

    Exclusion globExclAsteriskBeginning("*/foo");
    EXPECT_EQ(globExclAsteriskBeginning.type(), GLOB);
    EXPECT_TRUE(globExclAsteriskBeginning.appliesToPath("/tmp/foo"));
    EXPECT_TRUE(globExclAsteriskBeginning.appliesToPath("/tmp/bar/foo"));
    EXPECT_FALSE(globExclAsteriskBeginning.appliesToPath("/tmp/foo/bar"));

    Exclusion globExclQuestionMarkEnd("/var/log/syslog.?");
    EXPECT_EQ(globExclQuestionMarkEnd.type(), GLOB);
    EXPECT_TRUE(globExclQuestionMarkEnd.appliesToPath("/var/log/syslog.1"));
    EXPECT_FALSE(globExclQuestionMarkEnd.appliesToPath("/var/log/syslog."));

    Exclusion globExclQuestionMarkMiddle("/tmp/sh?t/happens");
    EXPECT_EQ(globExclQuestionMarkMiddle.type(), GLOB);
    EXPECT_TRUE(globExclQuestionMarkMiddle.appliesToPath("/tmp/shut/happens"));
    EXPECT_TRUE(globExclQuestionMarkMiddle.appliesToPath("/tmp/shot/happens"));
    EXPECT_FALSE(globExclQuestionMarkMiddle.appliesToPath("/tmp/spit/happens"));

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

TEST(Exclusion, TestFilenameTypes) // NOLINT
{
    Exclusion suffixExcl("foo.txt");
    EXPECT_EQ(suffixExcl.type(), FILENAME);
    EXPECT_EQ(suffixExcl.path(), "/foo.txt");
    EXPECT_TRUE(suffixExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(suffixExcl.appliesToPath("/tmp/bar/foo.txt.backup"));
}

TEST(Exclusion, TestInvalidTypes) // NOLINT
{
    Exclusion invalidExcl("");
    EXPECT_EQ(invalidExcl.type(), INVALID);
}