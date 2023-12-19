// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "common/Exclusion.h"

#include <gtest/gtest.h>

using namespace common;

namespace
{
    CachedPath cp(const char* p)
    {
        return CachedPath{std::string{p}};
    }

    bool appliesToCpFile(const Exclusion& ex, const char* p)
    {
        auto cachedPath = cp(p);
        return ex.appliesToPath(cachedPath, false, true);
    }
}

TEST(Exclusion, TestStemTypes)
{
    Exclusion rootExcl("/");
    EXPECT_EQ(rootExcl.type(), STEM);
    EXPECT_EQ(rootExcl.path(), "/");
    EXPECT_EQ(rootExcl.displayPath(), "/");
    EXPECT_TRUE(rootExcl.appliesToPath("/", true));
    EXPECT_FALSE(rootExcl.appliesToPath("~/", true));
    EXPECT_FALSE(rootExcl.appliesToPath("~", true));
    EXPECT_TRUE(rootExcl.appliesToPath("/tmp/", true));
    EXPECT_TRUE(rootExcl.appliesToPath("/wunde/foo/bar", true));
    EXPECT_TRUE(rootExcl.appliesToPath("/tmp/foo/bar", true));
    EXPECT_TRUE(appliesToCpFile(rootExcl, "/tmp/eicar.com"));

    Exclusion singleStemExcl("/tmp/");
    EXPECT_EQ(singleStemExcl.type(), STEM);
    EXPECT_EQ(singleStemExcl.path(), "/tmp/");
    EXPECT_EQ(singleStemExcl.displayPath(), "/tmp/");
    EXPECT_FALSE(singleStemExcl.appliesToPath("/"));
    EXPECT_FALSE(singleStemExcl.appliesToPath("/tmp"));
    EXPECT_TRUE(singleStemExcl.appliesToPath("/tmp/foo/"));
    EXPECT_FALSE(singleStemExcl.appliesToPath("/", true));
    EXPECT_TRUE(singleStemExcl.appliesToPath("/tmp", true));
    EXPECT_TRUE(singleStemExcl.appliesToPath("/tmp/foo/", true));
    EXPECT_TRUE(appliesToCpFile(singleStemExcl, "/tmp/eicar.com"));

    Exclusion stemExcl("/multiple/nested/dirs/");
    EXPECT_EQ(stemExcl.type(), STEM);
    EXPECT_EQ(stemExcl.path(), "/multiple/nested/dirs/");
    EXPECT_EQ(stemExcl.displayPath(), "/multiple/nested/dirs/");
    EXPECT_TRUE(stemExcl.appliesToPath("/multiple/nested/dirs/foo"));
    EXPECT_TRUE(stemExcl.appliesToPath("/multiple/nested/dirs/foo/bar"));
    EXPECT_FALSE(stemExcl.appliesToPath("/multiple/nested/dirs"));
    EXPECT_TRUE(stemExcl.appliesToPath("/multiple/nested/dirs", true, false)); // If we know dirs is a directory, then we can exclude it
    EXPECT_FALSE(stemExcl.appliesToPath("/multiple/nested/dirs", false, false));
    EXPECT_FALSE(stemExcl.appliesToPath("/multiple/nested/dirt", true, false));
    EXPECT_FALSE(stemExcl.appliesToPath("/multiple/nested/dirst", true, false));
    EXPECT_FALSE(stemExcl.appliesToPath("/multiple/nested/dis", true, false));
    EXPECT_TRUE(appliesToCpFile(stemExcl, "/multiple/nested/dirs/foo/bar"));

    {
        Exclusion rootExclWithAsterisk("/*");
        EXPECT_EQ(rootExclWithAsterisk.type(), STEM);
        EXPECT_EQ(rootExclWithAsterisk.path(), "/");
        EXPECT_EQ(rootExclWithAsterisk.displayPath(), "/*");
        EXPECT_TRUE(rootExclWithAsterisk.appliesToPath("/tmp/foo/bar"));
        EXPECT_TRUE(appliesToCpFile(rootExclWithAsterisk, "/multiple/nested/dirs/foo/bar"));
    }

    {
        Exclusion stemExclWithAsterisk("/multiple/nested/dirs/*");
        EXPECT_EQ(stemExclWithAsterisk.type(), STEM);
        EXPECT_EQ(stemExclWithAsterisk.path(), "/multiple/nested/dirs/");
        EXPECT_EQ(stemExclWithAsterisk.displayPath(), "/multiple/nested/dirs/*");
        EXPECT_TRUE(stemExclWithAsterisk.appliesToPath("/multiple/nested/dirs/foo"));
        EXPECT_TRUE(stemExclWithAsterisk.appliesToPath("/multiple/nested/dirs/foo/bar"));
        EXPECT_FALSE(stemExclWithAsterisk.appliesToPath("/multiple/nested/dirs"));
        EXPECT_TRUE(appliesToCpFile(stemExclWithAsterisk, "/multiple/nested/dirs/foo/bar"));
    }
}

TEST(Exclusion, TestFullpathTypes)
{
    {
        Exclusion fullpathExcl("/tmp/foo.txt");
        EXPECT_EQ(fullpathExcl.type(), FULLPATH);
        EXPECT_EQ(fullpathExcl.path(), "/tmp/foo.txt");
        EXPECT_EQ(fullpathExcl.displayPath(), "/tmp/foo.txt");
        EXPECT_TRUE(fullpathExcl.appliesToPath("/tmp/foo.txt"));
        EXPECT_FALSE(fullpathExcl.appliesToPath("/tmp/bar.txt"));
        EXPECT_FALSE(fullpathExcl.appliesToPath("/tmp/foo.txt/bar"));
        EXPECT_TRUE(appliesToCpFile(fullpathExcl, "/tmp/foo.txt"));
        EXPECT_FALSE(appliesToCpFile(fullpathExcl, "/tmp/foo.txt/bar"));
    }

    {
        Exclusion dirpathExcl("/tmp/foo");
        EXPECT_EQ(dirpathExcl.type(), FULLPATH);
        EXPECT_EQ(dirpathExcl.path(), "/tmp/foo");
        EXPECT_EQ(dirpathExcl.displayPath(), "/tmp/foo");
        EXPECT_TRUE(dirpathExcl.appliesToPath("/tmp/foo"));
        EXPECT_FALSE(dirpathExcl.appliesToPath("/tmp/foo", true));
        EXPECT_FALSE(dirpathExcl.appliesToPath("/tmp/foo/bar"));
        EXPECT_TRUE(appliesToCpFile(dirpathExcl, "/tmp/foo"));
        EXPECT_FALSE(appliesToCpFile(dirpathExcl, "/tmp/foo/bar"));
    }
}

TEST(Exclusion, TestGlobTypes)
{
    {
        Exclusion globExclAsteriskEnd("/tmp/foo*");
        EXPECT_EQ(globExclAsteriskEnd.type(), GLOB);
        EXPECT_EQ(globExclAsteriskEnd.path(), "/tmp/foo*");
        EXPECT_EQ(globExclAsteriskEnd.displayPath(), "/tmp/foo*");
        EXPECT_TRUE(globExclAsteriskEnd.appliesToPath("/tmp/foobar"));
        EXPECT_TRUE(globExclAsteriskEnd.appliesToPath("/tmp/foo"));
        EXPECT_FALSE(globExclAsteriskEnd.appliesToPath("/tmp/fo"));
        EXPECT_TRUE(appliesToCpFile(globExclAsteriskEnd, "/tmp/foobar"));
        EXPECT_TRUE(appliesToCpFile(globExclAsteriskEnd, "/tmp/foo/bar"));
        EXPECT_TRUE(appliesToCpFile(globExclAsteriskEnd, "/tmp/foo"));
        EXPECT_FALSE(appliesToCpFile(globExclAsteriskEnd, "/tmp/fo"));
    }

    {
        Exclusion globExclAsteriskBeginning("*/foo");
        EXPECT_EQ(globExclAsteriskBeginning.type(), FILENAME);
        EXPECT_EQ(globExclAsteriskBeginning.path(), "/foo");
        EXPECT_EQ(globExclAsteriskBeginning.displayPath(), "*/foo");
        EXPECT_TRUE(globExclAsteriskBeginning.appliesToPath("/tmp/foo"));
        EXPECT_TRUE(globExclAsteriskBeginning.appliesToPath("/tmp/bar/foo"));
        EXPECT_FALSE(globExclAsteriskBeginning.appliesToPath("/tmp/foo/bar"));
        EXPECT_TRUE(appliesToCpFile(globExclAsteriskBeginning, "/tmp/foo"));
        EXPECT_TRUE(appliesToCpFile(globExclAsteriskBeginning, "/tmp/bar/foo"));
        EXPECT_FALSE(appliesToCpFile(globExclAsteriskBeginning, "/tmp/bar/barfoo"));
        EXPECT_FALSE(appliesToCpFile(globExclAsteriskBeginning, "/tmp/foo/bar"));
    }

    {
        Exclusion dirExcl("*/bar/");
        EXPECT_EQ(dirExcl.type(), RELATIVE_STEM);
        EXPECT_EQ(dirExcl.path(), "/bar/");
        EXPECT_EQ(dirExcl.displayPath(), "*/bar/");
        EXPECT_TRUE(dirExcl.appliesToPath("/tmp/bar/foo.txt"));
        EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foobar/foo.txt"));
        EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foo/bar"));
        EXPECT_TRUE(appliesToCpFile(dirExcl, "/tmp/bar/foo"));
        EXPECT_TRUE(appliesToCpFile(dirExcl, "/tmp/foo/bar/foo"));
        EXPECT_FALSE(appliesToCpFile(dirExcl, "/tmp/barbarfoo"));
        EXPECT_FALSE(appliesToCpFile(dirExcl, "/tmp/foo/barbarfoo"));
    }

    Exclusion dirExclAsteriskEnd("bar/*");
    EXPECT_EQ(dirExclAsteriskEnd.type(), RELATIVE_STEM);
    EXPECT_EQ(dirExclAsteriskEnd.path(), "/bar/");
    EXPECT_EQ(dirExclAsteriskEnd.displayPath(), "bar/*");
    EXPECT_TRUE(dirExclAsteriskEnd.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirExclAsteriskEnd.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirExclAsteriskEnd.appliesToPath("/tmp/foo/bar"));

    Exclusion dirExclBothAsterisks("*/bar/*");
    EXPECT_EQ(dirExclBothAsterisks.type(), RELATIVE_STEM);
    EXPECT_EQ(dirExclBothAsterisks.path(), "/bar/");
    EXPECT_EQ(dirExclBothAsterisks.displayPath(), "*/bar/*");
    EXPECT_TRUE(dirExclBothAsterisks.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirExclBothAsterisks.appliesToPath("/tmp/foobar/foo.txt"));
    EXPECT_FALSE(dirExclBothAsterisks.appliesToPath("/tmp/foo/bar"));

    {
        Exclusion dirAndFileExcl("*/bar/foo.txt");
        EXPECT_EQ(dirAndFileExcl.type(), RELATIVE_PATH);
        EXPECT_EQ(dirAndFileExcl.path(), "/bar/foo.txt");
        EXPECT_EQ(dirAndFileExcl.displayPath(), "*/bar/foo.txt");
        EXPECT_TRUE(dirAndFileExcl.appliesToPath("/tmp/bar/foo.txt"));
        EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foobar/foo.txt"));
        EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foo/bar"));
        EXPECT_TRUE(appliesToCpFile(dirAndFileExcl, "/tmp/bar/foo.txt"));
        EXPECT_TRUE(appliesToCpFile(dirAndFileExcl, "/var/bar/foo.txt"));
        EXPECT_FALSE(appliesToCpFile(dirAndFileExcl, "/var/barfoo.txt"));
        EXPECT_FALSE(appliesToCpFile(dirAndFileExcl, "/var/bar/foo.com"));
    }

    {
        Exclusion relativeGlob("tmp/foo*");
        EXPECT_EQ(relativeGlob.type(), RELATIVE_GLOB);
        EXPECT_EQ(relativeGlob.path(), "*/tmp/foo*");
        EXPECT_EQ(relativeGlob.displayPath(), "tmp/foo*");
        EXPECT_TRUE(relativeGlob.appliesToPath("/tmp/foo/bar"));
        EXPECT_FALSE(relativeGlob.appliesToPath("/tmp/bar/foo/"));
        EXPECT_FALSE(relativeGlob.appliesToPath("/home/dev/tmp/bar/foo/"));
        EXPECT_TRUE(appliesToCpFile(relativeGlob, "/tmp/foo.txt"));
        EXPECT_TRUE(appliesToCpFile(relativeGlob, "/var/tmp/foo.txt"));
        EXPECT_TRUE(appliesToCpFile(relativeGlob, "/var/tmp/foo.com"));
        EXPECT_TRUE(appliesToCpFile(relativeGlob, "/var/tmp/fooBAR.com"));
    }

    Exclusion globExclQuestionMarkEnd("/var/log/syslog.?");
    EXPECT_EQ(globExclQuestionMarkEnd.type(), GLOB);
    EXPECT_EQ(globExclQuestionMarkEnd.path(), "/var/log/syslog.?");
    EXPECT_EQ(globExclQuestionMarkEnd.displayPath(), "/var/log/syslog.?");
    EXPECT_TRUE(globExclQuestionMarkEnd.appliesToPath("/var/log/syslog.1"));
    EXPECT_FALSE(globExclQuestionMarkEnd.appliesToPath("/var/log/syslog."));

    Exclusion globExclQuestionMarkMiddle("/tmp/sh?t/happens");
    EXPECT_EQ(globExclQuestionMarkMiddle.type(), GLOB);
    EXPECT_EQ(globExclQuestionMarkMiddle.path(), "/tmp/sh?t/happens");
    EXPECT_EQ(globExclQuestionMarkMiddle.displayPath(), "/tmp/sh?t/happens");
    EXPECT_TRUE(globExclQuestionMarkMiddle.appliesToPath("/tmp/shut/happens"));
    EXPECT_TRUE(globExclQuestionMarkMiddle.appliesToPath("/tmp/shot/happens"));
    EXPECT_FALSE(globExclQuestionMarkMiddle.appliesToPath("/tmp/spit/happens"));

    Exclusion singleMiddleGlobExcl("/tmp*/foo/");
    EXPECT_EQ(singleMiddleGlobExcl.type(), GLOB);
    EXPECT_EQ(singleMiddleGlobExcl.path(), "/tmp*/foo/*");
    EXPECT_EQ(singleMiddleGlobExcl.displayPath(), "/tmp*/foo/");
    EXPECT_TRUE(singleMiddleGlobExcl.appliesToPath("/tmp/foo/bar"));
    EXPECT_TRUE(singleMiddleGlobExcl.appliesToPath("/tmp/bar/foo/"));
    EXPECT_TRUE(singleMiddleGlobExcl.appliesToPath("/tmp/foo/bar/rrr/"));
    EXPECT_FALSE(singleMiddleGlobExcl.appliesToPath("/home/dev/tmp/bar/foo/"));

    Exclusion singleBeginGlobExcl("*tmp/foo/");
    EXPECT_EQ(singleBeginGlobExcl.type(), GLOB);
    EXPECT_EQ(singleBeginGlobExcl.path(), "*tmp/foo/*");
    EXPECT_EQ(singleBeginGlobExcl.displayPath(), "*tmp/foo/");
    EXPECT_TRUE(singleBeginGlobExcl.appliesToPath("/tmp/foo/bar"));
    EXPECT_TRUE(singleBeginGlobExcl.appliesToPath("/tmp/foo/bar/rrrr/"));
    EXPECT_FALSE(singleBeginGlobExcl.appliesToPath("/tmp/bar/foo/"));
    EXPECT_FALSE(singleBeginGlobExcl.appliesToPath("/home/dev/tmp/bar/foo/"));

    Exclusion doubleGlobExcl("/tmp*/foo/*");
    EXPECT_EQ(doubleGlobExcl.type(), GLOB);
    EXPECT_EQ(doubleGlobExcl.path(), "/tmp*/foo/*");
    EXPECT_EQ(doubleGlobExcl.displayPath(), "/tmp*/foo/*");
    EXPECT_TRUE(doubleGlobExcl.appliesToPath("/tmp/foo/bar"));
    EXPECT_TRUE(doubleGlobExcl.appliesToPath("/tmp/bar/foo/"));
    EXPECT_FALSE(doubleGlobExcl.appliesToPath("/home/dev/tmp/bar/foo/"));

    Exclusion regexMetaCharExcl("/tmp/regex[^\\\\]+$filename*");
    EXPECT_EQ(regexMetaCharExcl.type(), GLOB);
    EXPECT_EQ(regexMetaCharExcl.path(), "/tmp/regex[^\\\\]+$filename*");
    EXPECT_EQ(regexMetaCharExcl.displayPath(), "/tmp/regex[^\\\\]+$filename*");
    EXPECT_TRUE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\]+$filename"));
    EXPECT_TRUE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\]+$filename.txt"));
    EXPECT_FALSE(regexMetaCharExcl.appliesToPath("/tmp/regex[^\\\\+$filename.txt"));

    Exclusion regexMetaCharExcl2("/tmp/(e|E)ic{2,3}ar*");
    EXPECT_EQ(regexMetaCharExcl2.type(), GLOB);
    EXPECT_EQ(regexMetaCharExcl2.path(), "/tmp/(e|E)ic{2,3}ar*");
    EXPECT_EQ(regexMetaCharExcl2.displayPath(), "/tmp/(e|E)ic{2,3}ar*");
    EXPECT_TRUE(regexMetaCharExcl2.appliesToPath("/tmp/(e|E)ic{2,3}ar.com"));
    EXPECT_TRUE(regexMetaCharExcl2.appliesToPath(cp("/tmp/(e|E)ic{2,3}ar.com"), false, true));
    EXPECT_TRUE(appliesToCpFile(regexMetaCharExcl2, "/tmp/(e|E)ic{2,3}ar.com"));
    EXPECT_FALSE(appliesToCpFile(regexMetaCharExcl2, "/tmp/eiccar.com"));
}

TEST(Exclusion, MiddleGlobDoesNotMatchLongerString)
{
    Exclusion ex{"/tmp/a*b"};
    EXPECT_TRUE(ex.appliesToPath("/tmp/aab", false, true));
    EXPECT_TRUE(ex.appliesToPath("/tmp/accccddddb", false, true));
    EXPECT_FALSE(ex.appliesToPath("/tmp/accccddddb/another_thing", false, true));
    EXPECT_TRUE(ex.appliesToPath("/tmp/accccddddb/another_thing_b", false, true));
}

TEST(Exclusion, AbsolutePathWithDirectoryNameSuffix)
{
    // /directory/*.directorynamesuffix/
    Exclusion ex("/lib/*.so/");
    EXPECT_EQ(ex.type(), GLOB);
    EXPECT_EQ(ex.displayPath(), "/lib/*.so/");
    EXPECT_TRUE(ex.appliesToPath("/lib/foo.so/bar"));
    EXPECT_TRUE(ex.appliesToPath(cp("/lib/foo.so/bar"), false, true));
    EXPECT_TRUE(appliesToCpFile(ex, "/lib/bardffds.so/anythingfile"));
    EXPECT_TRUE(ex.appliesToPath("/lib/foo/bar.so/baz"));
    EXPECT_FALSE(ex.appliesToPath("/lib/foo.so"));
    // Special-directory handling only applies to stems, but logically should apply here as well:
//    EXPECT_TRUE(ex.appliesToPath("/lib/foo.so", true, false));
}

TEST(Exclusion, AbsolutePathWithDirectoryNamePrefix)
{
    // /directory/directorynameprefix.*/
    Exclusion ex("/lib/libz.*/");
    EXPECT_EQ(ex.type(), GLOB);
    EXPECT_EQ(ex.displayPath(), "/lib/libz.*/");
    EXPECT_TRUE(ex.appliesToPath("/lib/libz.so/foo"));
    EXPECT_TRUE(appliesToCpFile(ex, "/lib/libz.so/foo"));
    EXPECT_TRUE(ex.appliesToPath("/lib/libz.so.1/bar"));
    EXPECT_FALSE(ex.appliesToPath("/lib/foo.so"));
    EXPECT_FALSE(ex.appliesToPath("/lib/libz.so.1"));
    // Special-directory handling only applies to stems, but logically should apply here as well:
    //    EXPECT_TRUE(ex.appliesToPath("/lib/libz.so.1", true, false));
}

TEST(Exclusion, TestFilenameTypes)
{
    Exclusion filenameExcl("foo.txt");
    EXPECT_EQ(filenameExcl.type(), FILENAME);
    EXPECT_EQ(filenameExcl.path(), "/foo.txt");
    EXPECT_EQ(filenameExcl.displayPath(), "foo.txt");
    EXPECT_TRUE(filenameExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(appliesToCpFile(filenameExcl, "/tmp/bar/foo.txt"));
    EXPECT_FALSE(filenameExcl.appliesToPath("/tmp/bar/foo.txt.backup"));

    Exclusion filename2Excl("foo");
    EXPECT_EQ(filename2Excl.type(), FILENAME);
    EXPECT_EQ(filename2Excl.path(), "/foo");
    EXPECT_EQ(filename2Excl.displayPath(), "foo");
    EXPECT_TRUE(filename2Excl.appliesToPath("/tmp/bar/foo"));
    EXPECT_TRUE(appliesToCpFile(filename2Excl, "/tmp/bar/foo"));
    EXPECT_FALSE(filename2Excl.appliesToPath("/tmp/foo/bar"));
    EXPECT_FALSE(filename2Excl.appliesToPath("/tmp/barfoo"));
}

TEST(Exclusion, TestRelativeTypes)
{
    {
        // Directory Name
        Exclusion dirExcl("bar/");
        EXPECT_EQ(dirExcl.type(), RELATIVE_STEM);
        EXPECT_EQ(dirExcl.path(), "/bar/");
        EXPECT_EQ(dirExcl.displayPath(), "bar/");
        EXPECT_TRUE(dirExcl.appliesToPath("/tmp/bar/foo.txt"));
        EXPECT_TRUE(appliesToCpFile(dirExcl, "/tmp/bar/foo.txt"));
        EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foobar/foo.txt"));
        EXPECT_FALSE(dirExcl.appliesToPath("/tmp/foo/bar"));
        EXPECT_FALSE(appliesToCpFile(dirExcl, "/var/log/dmesg.txt"));
    }

    {
        Exclusion dirAndFileExcl("bar/foo.txt");
        EXPECT_EQ(dirAndFileExcl.type(), RELATIVE_PATH);
        EXPECT_EQ(dirAndFileExcl.path(), "/bar/foo.txt");
        EXPECT_EQ(dirAndFileExcl.displayPath(), "bar/foo.txt");
        EXPECT_TRUE(dirAndFileExcl.appliesToPath("/tmp/bar/foo.txt"));
        EXPECT_TRUE(appliesToCpFile(dirAndFileExcl, "/tmp/bar/foo.txt"));
        EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foobar/foo.txt"));
        EXPECT_FALSE(dirAndFileExcl.appliesToPath("/tmp/foo/bar"));
        EXPECT_FALSE(appliesToCpFile(dirAndFileExcl, "/var/log/dmesg.txt"));
    }
}

TEST(Exclusion, RelativePathToADirectory)
{
    // Relative path to a directory
    Exclusion dir2Excl("foo/bar/");
    EXPECT_EQ(dir2Excl.type(), RELATIVE_STEM);
    EXPECT_EQ(dir2Excl.path(), "/foo/bar/");
    EXPECT_EQ(dir2Excl.displayPath(), "foo/bar/");
    EXPECT_TRUE(dir2Excl.appliesToPath("/foo/bar/abc.txt"));
    EXPECT_TRUE(appliesToCpFile(dir2Excl, "/foo/bar/abc.txt"));
    EXPECT_TRUE(dir2Excl.appliesToPath("/baz/foo/bar/abc.txt"));
    EXPECT_FALSE(dir2Excl.appliesToPath("/baz/foobar/abc.txt"));
    EXPECT_FALSE(dir2Excl.appliesToPath("/baz/foofoo/bar/abc.txt"));
    EXPECT_FALSE(dir2Excl.appliesToPath("/baz/foo/bar"));
    // Special-directory handling only applies to stems, but logically should apply here as well:
    //    EXPECT_TRUE(dir2Excl.appliesToPath("/baz/foo/bar", true, false));
    EXPECT_FALSE(dir2Excl.appliesToPath("/baz/foo/bar", false, false));
    EXPECT_FALSE(dir2Excl.appliesToPath("/baz/foo/bar", false, true));
    EXPECT_FALSE(dir2Excl.appliesToPath(cp("/baz/foo/bar"), false, true));
}

TEST(Exclusion, TestRelativeGlobTypes)
{
    Exclusion filenameMatchAnyExcl("foo.*");
    EXPECT_EQ(filenameMatchAnyExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(filenameMatchAnyExcl.path(), "*/foo.*");
    EXPECT_EQ(filenameMatchAnyExcl.displayPath(), "foo.*");
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(appliesToCpFile(filenameMatchAnyExcl, "/tmp/bar/foo.txt"));
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo.bar.txt"));
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo."));
    EXPECT_FALSE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/foo"));
    EXPECT_TRUE(filenameMatchAnyExcl.appliesToPath("/tmp/foo.foo/bar.txt"));
    EXPECT_FALSE(filenameMatchAnyExcl.appliesToPath("/tmp/bar/barfoo.txt"));
    EXPECT_FALSE(appliesToCpFile(filenameMatchAnyExcl, "/foo/foobar/fooo.x/file"));

    Exclusion filenameMatchOneExcl("f?o.txt");
    EXPECT_EQ(filenameMatchOneExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(filenameMatchOneExcl.path(), "*/f?o.txt");
    EXPECT_EQ(filenameMatchOneExcl.displayPath(), "f?o.txt");
    EXPECT_TRUE(filenameMatchOneExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(appliesToCpFile(filenameMatchOneExcl, "/tmp/bar/foo.txt"));
    EXPECT_TRUE(filenameMatchOneExcl.appliesToPath("/tmp/bar/f.o.txt"));
    EXPECT_TRUE(filenameMatchOneExcl.appliesToPath("/tmp/bar/f/o.txt"));
    EXPECT_FALSE(filenameMatchOneExcl.appliesToPath("/tmp/bar/fo.txt"));
    EXPECT_FALSE(filenameMatchOneExcl.appliesToPath("/tmp/bar/fzzo.txt"));

    Exclusion filenameMatchThreeExcl("foo.???");
    EXPECT_EQ(filenameMatchThreeExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(filenameMatchThreeExcl.path(), "*/foo.???");
    EXPECT_EQ(filenameMatchThreeExcl.displayPath(), "foo.???");
    EXPECT_TRUE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(appliesToCpFile(filenameMatchThreeExcl, "/tmp/bar/foo.txt"));
    EXPECT_FALSE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.tt"));
    EXPECT_FALSE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.txtz"));
    EXPECT_FALSE(filenameMatchThreeExcl.appliesToPath("/tmp/bar/foo.txt/bar"));


    Exclusion dirMatchOneExcl("b?r/");
    EXPECT_EQ(dirMatchOneExcl.type(), RELATIVE_GLOB);
    EXPECT_EQ(dirMatchOneExcl.path(), "*/b?r/*");
    EXPECT_EQ(dirMatchOneExcl.displayPath(), "b?r/");
    EXPECT_TRUE(dirMatchOneExcl.appliesToPath("/tmp/bar/foo.txt"));
    EXPECT_TRUE(appliesToCpFile(dirMatchOneExcl, "/tmp/bar/foo.txt"));
    EXPECT_FALSE(dirMatchOneExcl.appliesToPath("/tmp/br/foo.txt"));
    EXPECT_FALSE(dirMatchOneExcl.appliesToPath("/tmp/blar/foo.txt"));

}

TEST(Exclusion, TestInvalidTypes)
{
    Exclusion invalidExcl("");
    EXPECT_EQ(invalidExcl.type(), INVALID);
    EXPECT_FALSE(invalidExcl.appliesToPath("/tmp/foo.txt"));
}

TEST(Exclusion, comparison)
{
    Exclusion filenameExcl("foo.txt");
    EXPECT_EQ(filenameExcl.type(), FILENAME);
    Exclusion dir2Excl("foo/bar/");
    EXPECT_EQ(dir2Excl.type(), RELATIVE_STEM);
    Exclusion dirMatchOneExcl("b?r/");
    EXPECT_EQ(dirMatchOneExcl.type(), RELATIVE_GLOB);
    Exclusion glob("/lib/libz.*/");
    EXPECT_EQ(glob.type(), GLOB);
    Exclusion glob2("/foo/lib/libz.*/");
    Exclusion glob3("/zoo/lib/libz.*/");
    Exclusion longFilename("abcdefghijklmnopqrstuvwxzyzfoo.txt");
    Exclusion ext{ "*.bat" };
    EXPECT_EQ(ext.type(), SUFFIX);

    EXPECT_LT(filenameExcl, glob);
    EXPECT_LT(dir2Excl, glob);
    EXPECT_LT(ext, glob);
    EXPECT_LT(filenameExcl, dirMatchOneExcl);
    EXPECT_LT(dir2Excl, dirMatchOneExcl);
    EXPECT_LT(glob, glob2);
    EXPECT_LT(longFilename, glob2);
    EXPECT_LT(glob2, glob3);
}

TEST(Exclusion, extension)
{
    {
        Exclusion ext{ "*.bat" };
        EXPECT_EQ(ext.type(), SUFFIX);
        EXPECT_TRUE(ext.appliesToPath("/tmp/bar/foo.bat"));
        EXPECT_TRUE(appliesToCpFile(ext, "/tmp/bar/foo.bat"));
        EXPECT_FALSE(ext.appliesToPath("/tmp/bar/foo.com"));
        EXPECT_FALSE(appliesToCpFile(ext, "/tmp/bar/foo.com"));
    }
}

TEST(Exclusion, dir_suffix)
{
    {
        Exclusion dir1{ "*/bar/foo.bat" };
        EXPECT_EQ(dir1.type(), RELATIVE_PATH); // Gets handled by code that takes off */ and puts / back on...
        EXPECT_TRUE(dir1.appliesToPath("/tmp/bar/foo.bat"));
        EXPECT_TRUE(appliesToCpFile(dir1, "/tmp/bar/foo.bat"));
        EXPECT_FALSE(dir1.appliesToPath("/tmp/bat/foo.bat"));
    }

    {
        Exclusion dir2{ "*bar/foo.bat" };
        EXPECT_EQ(dir2.type(), SUFFIX);
        EXPECT_TRUE(dir2.appliesToPath("/tmp/bar/foo.bat"));
        EXPECT_TRUE(appliesToCpFile(dir2, "/tmp/bar/foo.bat"));
        EXPECT_FALSE(dir2.appliesToPath("/tmp/bat/foo.bat"));
        EXPECT_FALSE(appliesToCpFile(dir2, "/tmp/bat/foo.bat"));
    }
}

TEST(Exclusion, eicar1path)
{
    Exclusion ex{"/tmp/eicar1"};
    ASSERT_EQ(ex.type(), FULLPATH);
    CachedPath p{std::string{"/tmp/eicar1"}};
    EXPECT_TRUE(ex.appliesToPath(p, false, true));
    EXPECT_TRUE(ex.appliesToPath("/tmp/eicar1", false, true));
    EXPECT_TRUE(ex.appliesToPath(cp("/tmp/eicar1"), false, true));
    EXPECT_TRUE(appliesToCpFile(ex, "/tmp/eicar1"));

    EXPECT_TRUE(ex.appliesToPath("/tmp/eicar1"));
    EXPECT_FALSE(ex.appliesToPath("/tmp/eicar2"));
    EXPECT_FALSE(appliesToCpFile(ex, "/tmp/eicar2"));
}

TEST(Exclusion, home1)
{
    Exclusion ex{"/home1/"};
    fs::path mp{"/home1"};
    EXPECT_TRUE(ex.appliesToPath(mp, true, false));
}

TEST(Exclusion, twoGlobAreEqual)
{
    const std::string glob{"/tmp/*/foo"};
    Exclusion one{glob};
    Exclusion two{glob};
    ASSERT_EQ(one.type(), GLOB);
    EXPECT_EQ(one, two);
}

TEST(Exclusion, combinedGlobs)
{
    Exclusion one{"/a/*/b"};
    Exclusion two{"/c/*/d"};
    std::vector<Exclusion> exclusions{one, two};
    Exclusion combined{exclusions};
    EXPECT_TRUE(one.appliesToPath("/a/foo/b", false, true));
    EXPECT_TRUE(combined.appliesToPath("/a/foo/b", false, true));
    EXPECT_EQ(combined.path(), "/a/.*/b|/c/.*/d");
    EXPECT_EQ(combined.displayPath(), "/a/.*/b|/c/.*/d");
}
