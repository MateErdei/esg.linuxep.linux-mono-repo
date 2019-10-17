/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/StringUtils.h>
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
                                                                                   { "pear;apple",
                                                                                     { "pear", "apple" } },
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
