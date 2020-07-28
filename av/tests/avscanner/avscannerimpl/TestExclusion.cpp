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
    EXPECT_TRUE(rootExcl.appliesToPath("/tmp/foo/bar"));

    Exclusion stemExcl("/multiple/nested/dirs/");
    EXPECT_EQ(stemExcl.type(), STEM);
    EXPECT_TRUE(stemExcl.appliesToPath("/multiple/nested/dirs/foo"));
    EXPECT_TRUE(stemExcl.appliesToPath("/multiple/nested/dirs/foo/bar"));
    EXPECT_FALSE(stemExcl.appliesToPath("/multiple/nested/dirs"));

    Exclusion rootExclWithAsterisk("/*");
    EXPECT_EQ(rootExclWithAsterisk.type(), STEM);
    EXPECT_EQ(rootExclWithAsterisk.path(), "/");
    EXPECT_TRUE(rootExclWithAsterisk.appliesToPath("/tmp/foo/bar"));

    Exclusion stemExclWithAsterisk("/multiple/nested/dirs/*");
    EXPECT_EQ(stemExclWithAsterisk.type(), STEM);
    EXPECT_EQ(stemExclWithAsterisk.path(), "/multiple/nested/dirs/");
    EXPECT_TRUE(stemExclWithAsterisk.appliesToPath("/multiple/nested/dirs/foo"));
    EXPECT_TRUE(stemExclWithAsterisk.appliesToPath("/multiple/nested/dirs/foo/bar"));
    EXPECT_FALSE(stemExclWithAsterisk.appliesToPath("/multiple/nested/dirs"));

}

TEST(Exclusion, TestFullpathTypes) // NOLINT
{
    Exclusion fullpathExcl("/tmp/foo.txt");
    EXPECT_EQ(fullpathExcl.type(), FULLPATH);
    EXPECT_TRUE(fullpathExcl.appliesToPath("/tmp/foo.txt"));
    EXPECT_FALSE(fullpathExcl.appliesToPath("/tmp/bar.txt"));
    EXPECT_FALSE(fullpathExcl.appliesToPath("/tmp/foo.txt/bar"));

    Exclusion dirpathExcl("/tmp/foo");
    EXPECT_EQ(dirpathExcl.type(), FULLPATH);
    EXPECT_TRUE(dirpathExcl.appliesToPath("/tmp/foo"));
    EXPECT_FALSE(dirpathExcl.appliesToPath("/tmp/foo", true));
    EXPECT_FALSE(dirpathExcl.appliesToPath("/tmp/foo/bar"));
}

TEST(Exclusion, TestGlobTypes) // NOLINT
{
    Exclusion globExclAsteriskEnd("/tmp/foo*");
    EXPECT_EQ(globExclAsteriskEnd.type(), GLOB);
    EXPECT_EQ(globExclAsteriskEnd.path(), "/tmp/foo*");
    EXPECT_TRUE(globExclAsteriskEnd.appliesToPath("/tmp/foobar"));
    EXPECT_TRUE(globExclAsteriskEnd.appliesToPath("/tmp/foo"));
    EXPECT_FALSE(globExclAsteriskEnd.appliesToPath("/tmp/fo"));

    Exclusion globExclAsteriskBeginning("*/foo");
    EXPECT_EQ(globExclAsteriskBeginning.type(), FILENAME);
    EXPECT_EQ(globExclAsteriskBeginning.path(), "/foo");
    EXPECT_TRUE(globExclAsteriskBeginning.appliesToPath("/tmp/foo"));
    EXPECT_TRUE(globExclAsteriskBeginning.appliesToPath("/tmp/bar/foo"));
    EXPECT_FALSE(globExclAsteriskBeginning.appliesToPath("/tmp/foo/bar"));

    Exclusion dirExcl("*/bar/");
    EXPECT_EQ(dirExcl.type(), RELATIVE_STEM);
    EXPECT_EQ(dirExcl.path(), "/bar/");
    EXPECT_TRUE(dirExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foo/bar"));

    Exclusion dirExclAsteriskEnd("bar/*");
    EXPECT_EQ(dirExclAsteriskEnd.type(), RELATIVE_STEM);
    EXPECT_EQ(dirExclAsteriskEnd.path(), "/bar/");
    EXPECT_TRUE(dirExclAsteriskEnd.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirExclAsteriskEnd.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirExclAsteriskEnd.appliesToPath("/tmp/foo/bar"));

    Exclusion dirExclBothAsterisks("*/bar/*");
    EXPECT_EQ(dirExclBothAsterisks.type(), RELATIVE_STEM);
    EXPECT_EQ(dirExclBothAsterisks.path(), "/bar/");
    EXPECT_TRUE(dirExclBothAsterisks.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirExclBothAsterisks.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirExclBothAsterisks.appliesToPath("/tmp/foo/bar"));

    Exclusion dirAndFileExcl("*/bar/foo.txt");
    EXPECT_EQ(dirAndFileExcl.type(), RELATIVE_PATH);
    EXPECT_EQ(dirAndFileExcl.path(), "/bar/foo.txt");
    EXPECT_TRUE(dirAndFileExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foo/bar"));

    Exclusion globExclQuestionMarkEnd("/var/log/syslog.?");
    EXPECT_EQ(globExclQuestionMarkEnd.type(), GLOB);
    EXPECT_EQ(globExclQuestionMarkEnd.path(), "/var/log/syslog.?");
    EXPECT_TRUE(globExclQuestionMarkEnd.appliesToPath("/var/log/syslog.1"));
    EXPECT_FALSE(globExclQuestionMarkEnd.appliesToPath("/var/log/syslog."));

    Exclusion globExclQuestionMarkMiddle("/tmp/sh?t/happens");
    EXPECT_EQ(globExclQuestionMarkMiddle.type(), GLOB);
    EXPECT_EQ(globExclQuestionMarkMiddle.path(), "/tmp/sh?t/happens");
    EXPECT_TRUE(globExclQuestionMarkMiddle.appliesToPath("/tmp/shut/happens"));
    EXPECT_TRUE(globExclQuestionMarkMiddle.appliesToPath("/tmp/shot/happens"));
    EXPECT_FALSE(globExclQuestionMarkMiddle.appliesToPath("/tmp/spit/happens"));

    Exclusion doubleGlobExcl("/tmp*/foo/*");
    EXPECT_EQ(doubleGlobExcl.type(), GLOB);
    EXPECT_EQ(doubleGlobExcl.path(), "/tmp*/foo/*");
    EXPECT_TRUE(doubleGlobExcl.appliesToPath("/tmp/foo/bar"));
    EXPECT_TRUE(doubleGlobExcl.appliesToPath("/tmp/bar/foo/"));
    EXPECT_FALSE(doubleGlobExcl.appliesToPath("/home/dev/tmp/bar/foo/"));

    Exclusion regexMetaCharExcl("/tmp/regex[^\\\\]+$filename*");
    EXPECT_EQ(regexMetaCharExcl.type(), GLOB);
    EXPECT_EQ(regexMetaCharExcl.path(), "/tmp/regex[^\\\\]+$filename*");
    EXPECT_TRUE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\]+$filename"));
    EXPECT_TRUE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\]+$filename.txt"));
    EXPECT_FALSE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\+$filename.txt"));

    Exclusion regexMetaCharExcl2("/tmp/(e|E)ic{2,3}ar*");
    EXPECT_EQ(regexMetaCharExcl2.type(), GLOB);
    EXPECT_EQ(regexMetaCharExcl2.path(), "/tmp/(e|E)ic{2,3}ar*");
    EXPECT_TRUE(regexMetaCharExcl2.appliesToPath("/tmp/(e|E)ic{2,3}ar.com"));
}

TEST(Exclusion, TestFilenameTypes) // NOLINT
{
    Exclusion filenameExcl("foo.txt");
    EXPECT_EQ(filenameExcl.type(), FILENAME);
    EXPECT_EQ(filenameExcl.path(), "/foo.txt");
    EXPECT_TRUE(filenameExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(filenameExcl.appliesToPath("/tmp/bar/foo.txt.backup"));

    Exclusion filename2Excl("foo");
    EXPECT_EQ(filename2Excl.type(), FILENAME);
    EXPECT_EQ(filename2Excl.path(), "/foo");
    EXPECT_TRUE(filename2Excl.appliesToPath("/tmp/bar/foo"));
    EXPECT_FALSE(filename2Excl.appliesToPath("/tmp/foo/bar"));
    EXPECT_FALSE(filename2Excl.appliesToPath("/tmp/barfoo"));
}

TEST(Exclusion, TestRelativeTypes) // NOLINT
{
    Exclusion dirExcl("bar/");
    EXPECT_EQ(dirExcl.type(), RELATIVE_STEM);
    EXPECT_EQ(dirExcl.path(), "/bar/");
    EXPECT_TRUE(dirExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foo/bar"));

    Exclusion dirAndFileExcl("bar/foo.txt");
    EXPECT_EQ(dirAndFileExcl.type(), RELATIVE_PATH);
    EXPECT_EQ(dirAndFileExcl.path(), "/bar/foo.txt");
    EXPECT_TRUE(dirAndFileExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foo/bar"));
}

TEST(Exclusion, TestRelativeGlobTypes) // NOLINT
{
    Exclusion filenameMatchAnyExcl("foo.*");
    EXPECT_EQ(filenameMatchAnyExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(filenameMatchAnyExcl.path(), "*/foo.*");
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo.bar.txt"));
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo."));
    EXPECT_FALSE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo"));
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/foo.foo/bar.txt"));
    EXPECT_FALSE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/barfoo.txt"));

    Exclusion filenameMatchOneExcl("f?o.txt");
    EXPECT_EQ(filenameMatchOneExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(filenameMatchOneExcl.path(), "*/f?o.txt");
    EXPECT_TRUE(filenameMatchOneExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(filenameMatchOneExcl.appliesToPath("/tmp/bar/f.o.txt"));
    EXPECT_TRUE(filenameMatchOneExcl.appliesToPath("/tmp/bar/f/o.txt"));
    EXPECT_FALSE(filenameMatchOneExcl.appliesToPath("/tmp/bar/fo.txt"));
    EXPECT_FALSE(filenameMatchOneExcl.appliesToPath("/tmp/bar/fzzo.txt"));

    Exclusion filenameMatchThreeExcl("foo.???");
    EXPECT_EQ(filenameMatchThreeExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(filenameMatchThreeExcl.path(), "*/foo.???");
    EXPECT_TRUE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.tt"));
    EXPECT_FALSE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.txtz"));
    EXPECT_FALSE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.txt/bar"));


    Exclusion dirMatchOneExcl("b?r/");
    EXPECT_EQ(dirMatchOneExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(dirMatchOneExcl.path(), "*/b?r/*");
    EXPECT_TRUE(dirMatchOneExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirMatchOneExcl.appliesToPath("/tmp/br/foo.txt"));
    EXPECT_FALSE(dirMatchOneExcl.appliesToPath("/tmp/blar/foo.txt"));

}

TEST(Exclusion, TestInvalidTypes) // NOLINT
{
    Exclusion invalidExcl("");
    EXPECT_EQ(invalidExcl.type(), INVALID);
    EXPECT_FALSE(invalidExcl.appliesToPath("/tmp/foo.txt"));
}