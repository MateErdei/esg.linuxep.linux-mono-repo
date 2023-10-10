// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/UtilityImpl/StringUtils.h"
#include <gtest/gtest.h>

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

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> contents{{"KEY = stuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_EQ(StringUtils::extractValueFromIniFile(filePath,"KEY"),"stuff");

}

TEST(TestStringUtils, extractValueFromIniFileThrowsIfFiledoesntExist) // NOLINT
{

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string filePath1 = "/tmp/file1";
    EXPECT_CALL(*filesystemMock, isFile(filePath1)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_THROW(StringUtils::extractValueFromIniFile(filePath1,"KEY"),std::runtime_error);
}
TEST(TestStringUtils, extractValueFromIniFileThrowsIfKeydoesntExist) // NOLINT
{

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> contents{{"KEY2 = stuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_EQ(StringUtils::extractValueFromIniFile(filePath,"KEY"),"");
}

TEST(TestStringUtils, extractValueFromConfigFileCanGetKeyValue) // NOLINT
{

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> contents{{"KEY=stuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_EQ(StringUtils::extractValueFromConfigFile(filePath,"KEY"),"stuff");

}

TEST(TestStringUtils, extractValueFromConfigFileThrowsIfFiledoesntExist) // NOLINT
{

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string filePath1 = "/tmp/file1";
    EXPECT_CALL(*filesystemMock, isFile(filePath1)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_THROW(StringUtils::extractValueFromConfigFile(filePath1,"KEY"),std::runtime_error);
}
TEST(TestStringUtils, extractValueFromConfigFileKeyDoesNotexist) // NOLINT
{

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> contents{{"KEY1=stuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_EQ(StringUtils::extractValueFromConfigFile(filePath,"KEY"),"");

}

TEST(TestStringUtils, extractValueFromConfigFileCanGetKeyValueWhenKeyIsDefinedMoreThanOnce) // NOLINT
{

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> contents{{"KEY=stuff"},{"KEY=notstuff"}} ;
    std::string filePath = "/tmp/file";

    EXPECT_CALL(*filesystemMock, isFile(filePath)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(filePath)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_EQ(StringUtils::extractValueFromConfigFile(filePath,"KEY"),"stuff");

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

TEST(TestStringUtils, stringToIntPositiveValuesReturnCorrectlyWithoutError) // NOLINT
{
    std::pair<int, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToInt("0"));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, 0);

    EXPECT_NO_THROW(result = StringUtils::stringToInt(std::to_string(INT_LEAST32_MAX)));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, INT_LEAST32_MAX);
}
TEST(TestStringUtils, stringToInt_StringStartingWithNumberReturnsNumberWithoutError) // NOLINT
{
    std::pair<int, std::string> result;

    // The follow demonstraights that stoi will return a valid value if the string starts with a number.
    EXPECT_NO_THROW(result = StringUtils::stringToInt("123hello"));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, 123);
}

TEST(TestStringUtils, stringToIntNegitiveValuesReturnCorrectlyWithoutError) // NOLINT
{
    std::pair<int, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToInt("-1"));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, -1);

    EXPECT_NO_THROW(result = StringUtils::stringToInt(std::to_string(INT_LEAST32_MIN)));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, INT_LEAST32_MIN);
}

TEST(TestStringUtils, stringToInt_StringValuesReturnCorrectlyWithError) // NOLINT
{
    std::pair<int, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToInt("hello"));
    EXPECT_EQ(result.second, "Failed to find integer from output: hello. Error message: stoi");
    EXPECT_EQ(result.first, 0);

    EXPECT_NO_THROW(result = StringUtils::stringToInt("h"));
    EXPECT_EQ(result.second, "Failed to find integer from output: h. Error message: stoi");
    EXPECT_EQ(result.first, 0);
}

TEST(TestStringUtils, stringToInt_OutofRangeValuesReturnCorrectlyWithError) // NOLINT
{
    std::pair<int, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToInt(std::to_string(INT_LEAST64_MAX)));
    EXPECT_EQ(result.second, "Failed to find integer from output: 9223372036854775807. Error message: stoi");
    EXPECT_EQ(result.first, 0);
}

TEST(TestStringUtils, stringToLongPositiveValuesReturnCorrectlyWithoutError) // NOLINT
{
    std::pair<long, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToLong("0"));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, 0);

    EXPECT_NO_THROW(result = StringUtils::stringToLong(std::to_string(INT_LEAST64_MAX)));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, INT_LEAST64_MAX);
}
TEST(TestStringUtils, stringToLong_StringStartingWithNumberReturnsNumberWithoutError) // NOLINT
{
    std::pair<long, std::string> result;

    // The follow demos that stol will return a valid value if the string starts with a number.
    EXPECT_NO_THROW(result = StringUtils::stringToLong("123hello"));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, 123);
}

TEST(TestStringUtils, stringToLongNegitiveValuesReturnCorrectlyWithoutError) // NOLINT
{
    std::pair<long, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToLong("-1"));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, -1);

    EXPECT_NO_THROW(result = StringUtils::stringToLong(std::to_string(INT_LEAST64_MIN)));
    EXPECT_EQ(result.second, "");
    EXPECT_EQ(result.first, INT_LEAST64_MIN);
}

TEST(TestStringUtils, stringToLong_StringValuesReturnCorrectlyWithError) // NOLINT
{
    std::pair<long, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToLong("hello"));
    EXPECT_EQ(result.second, "Failed to find integer from output: hello. Error message: stol");
    EXPECT_EQ(result.first, 0);

    EXPECT_NO_THROW(result = StringUtils::stringToLong("h"));
    EXPECT_EQ(result.second, "Failed to find integer from output: h. Error message: stol");
    EXPECT_EQ(result.first, 0);
}

TEST(TestStringUtils, stringToLong_OutofRangeValuesReturnCorrectlyWithError) // NOLINT
{
    std::pair<int, std::string> result;

    EXPECT_NO_THROW(result = StringUtils::stringToLong(std::to_string(UINT_LEAST64_MAX)));
    EXPECT_EQ(result.second, "Failed to find integer from output: 18446744073709551615. Error message: stol");
    EXPECT_EQ(result.first, 0);
}

TEST(TestStringUtils, stringToULongReturnCorrectlyWithoutError) // NOLINT
{
    unsigned long result;
    
    EXPECT_NO_THROW(result = StringUtils::stringToULong("0"));
    EXPECT_EQ(result, 0);

    EXPECT_NO_THROW(result = StringUtils::stringToULong(std::to_string(ULONG_MAX)));
    EXPECT_EQ(result, ULONG_MAX);
}

TEST(TestStringUtils, stringToULong_StringStartingWithNumberReturnsNumberWithoutError) // NOLINT
{
    unsigned long result;

    // The follow demos that stoul will return a valid value if the string starts with a number.
    EXPECT_NO_THROW(result = StringUtils::stringToULong("123hello"));
    EXPECT_EQ(result, 123);
}

TEST(TestStringUtils, stringToULong_StringValuesReturnCorrectlyWithError) // NOLINT
{
    EXPECT_THROW(StringUtils::stringToULong("hello"), std::runtime_error);
}

TEST(TestStringUtils, stringToULong_OutofRangeValuesReturnCorrectlyWithError) // NOLINT
{
    EXPECT_THROW(StringUtils::stringToULong("999999999999999999999999999999999999"), std::runtime_error);
}

TEST(TestStringUtils, rTrimRemovesWhiteSpaceFromRightSideOfString) // NOLINT
{
    std::string result = StringUtils::rTrim("1 space after ");
    ASSERT_EQ(result, "1 space after");

    result = StringUtils::rTrim("   3 spaces before 3 spaces after   ");
    ASSERT_EQ(result, "   3 spaces before 3 spaces after");
}

TEST(TestStringUtils, lTrimRemovesWhiteSpaceFromLeftSideOfString) // NOLINT
{
    std::string result = StringUtils::lTrim(" 1 space before");
    ASSERT_EQ(result, "1 space before");

    result = StringUtils::lTrim("   3 spaces before 3 spaces after   ");
    ASSERT_EQ(result, "3 spaces before 3 spaces after   ");
}

TEST(TestStringUtils, trimRemovesWhiteSpaceFromBothSidesOfString) // NOLINT
{
    std::string result = StringUtils::trim(" 1 space before");
    ASSERT_EQ(result, "1 space before");

    result = StringUtils::trim("1 space after ");
    ASSERT_EQ(result, "1 space after");

    result = StringUtils::trim("   3 spaces before 3 spaces after   ");
    ASSERT_EQ(result, "3 spaces before 3 spaces after");
}

TEST(TestStringUtils, rTrimRemovesCustomCharFromRightSideOfString) // NOLINT
{
    std::string result = StringUtils::rTrim("aaababaaa", [](char c) { return c == 'a'; });
    ASSERT_EQ(result, "aaabab");
}

TEST(TestStringUtils, lTrimRemovesCustomCharFromLeftSideOfString) // NOLINT
{
    std::string result = StringUtils::lTrim("aaababaaa", [](char c) { return c == 'a'; });
    ASSERT_EQ(result, "babaaa");
}

TEST(TestStringUtils, trimRemovesCustomCharFromBothSidesOfString) // NOLINT
{
    std::string result = StringUtils::trim("aaababaaa", [](char c) { return c == 'a'; });
    ASSERT_EQ(result, "bab");
}

TEST(TestStringUtils, toLowerMakesAllCharsLowerCase) // NOLINT
{
    std::string testStringMixed = "aSdFgHjK";
    std::string testStringMixedLowered = "asdfghjk";

    std::string testStringUpper = "QWERTY";
    std::string testStringUpperLowered = "qwerty";

    std::string testStringLower = "zxcvb";
    std::string testStringNotLetters = "?!~#256";

    ASSERT_EQ(StringUtils::toLower(testStringMixed), testStringMixedLowered);
    ASSERT_EQ(StringUtils::toLower(testStringUpper), testStringUpperLowered);
    ASSERT_EQ(StringUtils::toLower(testStringLower), testStringLower);
    ASSERT_EQ(StringUtils::toLower(testStringNotLetters), testStringNotLetters);
}

TEST(TestStringUtils, isPositiveInteger)
{
    ASSERT_EQ(StringUtils::isPositiveInteger("1"), true);
    ASSERT_EQ(StringUtils::isPositiveInteger(""), false);
    ASSERT_EQ(StringUtils::isPositiveInteger("123e"), false);
    ASSERT_EQ(StringUtils::isPositiveInteger("e"), false);
}