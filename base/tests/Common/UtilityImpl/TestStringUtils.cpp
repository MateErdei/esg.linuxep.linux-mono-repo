/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/UtilityImpl/StringUtils.h>

#include "../Helpers/MockFileSystem.h"
#include "../Helpers/FileSystemReplaceAndRestore.h"

#include <gtest/gtest.h>
#include "../Helpers/MockFileSystem.h"
#include "../Helpers/FileSystemReplaceAndRestore.h"
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>


using namespace Common::UtilityImpl;

TEST(TestStringUtils, endswith) // NOLINT
{
    EXPECT_TRUE(StringUtils::endswith("FOOBAR", "BAR"));
    EXPECT_TRUE(StringUtils::endswith("BAR", "BAR"));
    EXPECT_FALSE(StringUtils::endswith("FOOBAS", "BAR"));
    EXPECT_FALSE(StringUtils::endswith("", "BAR"));
    EXPECT_FALSE(StringUtils::endswith("F", "BAR"));
    EXPECT_TRUE(StringUtils::endswith("FOOBAR", ""));
    EXPECT_TRUE(StringUtils::endswith("BARFOOBAR", "BAR"));
    EXPECT_TRUE(StringUtils::endswith("stuff.json", ".json"));
    EXPECT_TRUE(StringUtils::endswith("stuff.json.json", ".json"));
    EXPECT_FALSE(StringUtils::endswith("stuff.json.other", ".json"));
}

TEST(TestStringUtils, splitString) // NOLINT
{
    std::vector<std::pair<std::string, std::vector<std::string>>> expectedResults{ { "a;b;c", { "a", "b", "c" } },
                                                                                   { "a;b;c;", { "a", "b", "c", "" } },
                                                                                   { "pear;apple", { "pear", "apple" } },
                                                                                   { "pear apple", { "pear apple" } },
                                                                                   { "", { "" } } };

    for (auto& entry : expectedResults)
    {
        std::string& inputString = entry.first;
        std::vector<std::string>& expectedSplitted = entry.second;
        EXPECT_EQ(expectedSplitted, StringUtils::splitString(inputString, ";"));
    }
}

TEST(TestStringUtils, startswith) // NOLINT
{
    EXPECT_TRUE(StringUtils::startswith("FOOBAR", "FOO"));
    EXPECT_TRUE(StringUtils::startswith("FOOBAR", ""));
    EXPECT_TRUE(StringUtils::startswith("FOOBAR", "F"));
    EXPECT_TRUE(StringUtils::startswith("", ""));
    EXPECT_FALSE(StringUtils::startswith("FOOBAR", "BAR"));
}

TEST(TestStringUtils, replace) // NOLINT
{
    EXPECT_EQ(StringUtils::replaceAll("FOO", "FOO", "BAR"), "BAR");
    EXPECT_EQ(StringUtils::replaceAll("FOO", "", ""), "FOO");
    EXPECT_EQ(StringUtils::replaceAll("FOOBARFOO", "BAR", "FOO"), "FOOFOOFOO");
    EXPECT_EQ(StringUtils::replaceAll("FOOBARFOOBARBAS", "BAR", "FOO"), "FOOFOOFOOFOOBAS");

    EXPECT_EQ(StringUtils::replaceAll("ABCDEFGH", "D", ""), "ABCEFGH");

    EXPECT_EQ(StringUtils::replaceAll("", "FOO", "BAR"), "");
    EXPECT_EQ(StringUtils::replaceAll("", "", ""), "");
}

TEST(TestStringUtils, orderedStringReplace) // NOLINT
{
    EXPECT_EQ(StringUtils::orderedStringReplace("Hello @@name@@", { { "@@name@@", "sophos" } }), "Hello sophos");
    EXPECT_EQ(
        StringUtils::orderedStringReplace("@a@ @b@", { { "@a@", "first" }, { "@b@", "second" } }), "first second");
    // it is ordered. It will only replace @a@ that it shown after @b@
    EXPECT_EQ(StringUtils::orderedStringReplace("@a@ @b@", { { "@b@", "second" }, { "@a@", "first" } }), "@a@ second");
    EXPECT_EQ(
        StringUtils::orderedStringReplace("@a@ @b@ @a@", { { "@b@", "second" }, { "@a@", "first" } }),
        "@a@ second first");
    // only first match is replaced
    EXPECT_EQ(StringUtils::orderedStringReplace("@a@ @a@", { { "@a@", "first" } }), "first @a@");

    // if the first element is not present, nothing changes
    EXPECT_EQ(StringUtils::orderedStringReplace("@a@ @a@", { { "@b@", "not there" }, { "@a@", "first" } }), "@a@ @a@");

    // key may be repeated
    EXPECT_EQ(StringUtils::orderedStringReplace("@a@ @a@", { { "@a@", "first" }, { "@a@", "other" } }), "first other");

    // empty always produces empty
    EXPECT_EQ(StringUtils::orderedStringReplace("", { { "@a@", "first" }, { "@a@", "other" } }), "");
}

TEST(TestStringUtils, enforceUTF8) // NOLINT
{
    EXPECT_THROW(StringUtils::enforceUTF8("\257"),std::invalid_argument);
    EXPECT_NO_THROW(StringUtils::enforceUTF8("FOOBAR"));
}

TEST(TestStringUtils, extractValueFromIniFileCanGetKeyValue) // NOLINT
{

    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> contents{{"KEY = stuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_EQ(StringUtils::extractValueFromIniFile(filePath,"KEY"),"stuff");

}

TEST(TestStringUtils, extractValueFromIniFileThrowsIfFiledoesntExist) // NOLINT
{

    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string filePath1 = "/tmp/file1";
    EXPECT_CALL(*filesystemMock, isFile(filePath1)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_THROW(StringUtils::extractValueFromIniFile(filePath1,"KEY"),std::runtime_error);
}
TEST(TestStringUtils, extractValueFromIniFileThrowsIfKeydoesntExist) // NOLINT
{

    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> contents{{"KEY2 = stuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_EQ(StringUtils::extractValueFromIniFile(filePath,"KEY"),"");
}

TEST(TestStringUtils, isVersionOlderthrowsOnNonVersionData) // NOLINT
{
    EXPECT_THROW(StringUtils::isVersionOlder("1.2","1.a"),std::invalid_argument);
    EXPECT_THROW(StringUtils::isVersionOlder("1.a","1.2"),std::invalid_argument);
    EXPECT_THROW(StringUtils::isVersionOlder("1.2","hello"),std::invalid_argument);
    EXPECT_THROW(StringUtils::isVersionOlder("hello","1.2"),std::invalid_argument);
    EXPECT_THROW(StringUtils::isVersionOlder("hello","stuff"),std::invalid_argument);

}

TEST(TestStringUtils, isVersionOlder) // NOLINT
{
    EXPECT_EQ(StringUtils::isVersionOlder("1.2","1.2"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2","1.2.3"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.3","1.2"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.0","1.2"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.3","1.2"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.","1.2"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("..","1.2"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2",".."),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.0.2.16","9.99.9"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("9.99.9","1.0.2.16"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("90.99.9","1.0.2.16"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("05.99.9","9.0.2.16"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("5000","5000.0.2.16"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("0000000000000000000000000000000000007.99.9","9.0.2.16"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("0000000000000000000000000000000000007.99.9","9.0.2.16"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("","9.0.2"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("9.0.2",""),true);
    EXPECT_EQ(StringUtils::isVersionOlder("..9.0","1"),true);
    // multiple separator dots evaluate to 0
    EXPECT_EQ(StringUtils::isVersionOlder(".....","........"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("........","....."),false);
    EXPECT_EQ(StringUtils::isVersionOlder("0.0.0","1"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1","0.0.0"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("9...3.1","9.3.1"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("9...3.1","9.2.1"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1","1"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.3","1.2.3"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.3.4","1.2.3.4"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("9","1"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1","9"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.9","1.2"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2","1.9"), false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.9","1.2.3"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.3","1.2.9"),false);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.3.9","1.2.3.4"),true);
    EXPECT_EQ(StringUtils::isVersionOlder("1.2.3.4","1.2.3.9"),false);

}