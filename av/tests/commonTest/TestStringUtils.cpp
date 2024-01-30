// Copyright 2020-2023 Sophos Limited. All rights reserved.


#include <gtest/gtest.h>

#include "common/StringUtils.h"

#include <log4cplus/loglevel.h>
#include "Common/Logging/SophosLoggerMacros.h"
#include "tests/common/LogInitializedTests.h"
#include <regex>


using namespace common;

namespace
{
    class TestStringUtils : public LogInitializedTests
    {};
}

TEST_F(TestStringUtils, TestXMLEscapeEscapesControlCharacters)
{
    std::string threatPath = "abc \1 \2 \3 \4 \5 \6"
                             " \016 \017 \020 \021 \022 \023 \024 \025 \026 \027"
                             " \030 \031 \032 \033 \034 \035 \036 \037"
                             " \177 \\ abc \a \b \t \n \v \f \r abc";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "abc \\001 \\002 \\003 \\004 \\005 \\006"
                          " \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027"
                          " \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037"
                          " \\177 \\\\ abc \\a \\b \\t \\n \\v \\f \\r abc");
}

TEST_F(TestStringUtils, TestXMLEscapeNotEscapingWeirdCharacters)
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6"
                             " \016 \017 \020 \021 \022 \023 \024 \025 \026 \027"
                             " \030 \031 \032 \033 \034 \035 \036 \037"
                             " \177 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "ありったけの夢をかき集め \\001 \\002 \\003 \\004 \\005 \\006"
                          " \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027"
                          " \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037"
                          " \\177 \\\\ Ἄνδρα μοι ἔννεπε \\a \\b \\t \\n \\v \\f \\r Ä Ö Ü ß");
}

TEST_F(TestStringUtils, TestXMLEscapeEscapesSpecialXMLCharacters)
{
    std::string threatPath = "abc & < > \' \" abc";
    escapeControlCharacters(threatPath, true);
    EXPECT_EQ(threatPath, "abc &amp; &lt; &gt; &apos; &quot; abc");
}

TEST_F(TestStringUtils, TestEscapeControlCharactersWithoutXLMEscaping)
{
    std::string threatPath = "abc & < > \' \" abc \1 \2 \3 \4 \5 \6 \7"
                             " \10 \11 \12 \13 \14 \15 \16 \17";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "abc & < > \' \" abc \\001 \\002 \\003 \\004 \\005 \\006 \\a"
                          " \\b \\t \\n \\v \\f \\r \\016 \\017");
}

TEST_F(TestStringUtils, TestEscapeEmptyString)
{
    std::string threatPath {};
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "");
}

TEST_F(TestStringUtils, TestToUtf8)
{
    std::string threatPath = "abc  \1 \2 \3 \4 \5 \6 \\ efg \a \b \t \n \v \f \r hik";
    std::string utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPath);
}

TEST_F(TestStringUtils, TestToUtf8WeirdCharacters)
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    std::string utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPath);
}

TEST_F(TestStringUtils, TestToUtf8FromEucJP)
{
    // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t euc-jp | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0xa4, 0xa2, 0xa4, 0xea, 0xa4, 0xc3, 0xa4, 0xbf, 0xa4, 0xb1, 0xa4, 0xce,
                                      0xcc, 0xb4, 0xa4, 0xf2, 0xa4, 0xab, 0xa4, 0xad, 0xbd, 0xb8, 0xa4, 0xe1 };
    std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

    std::string threatPathUtf8 = "ありったけの夢をかき集め";
    std::string utf8Path = toUtf8(threatPath, false);
    EXPECT_EQ(utf8Path, threatPathUtf8);

    threatPathUtf8 += " (EUC-JP)";
    utf8Path = toUtf8(threatPath, true);
    EXPECT_EQ(utf8Path, threatPathUtf8);

    utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPathUtf8);
}

TEST_F(TestStringUtils, TestToUtf8FromSJIS)
{
    // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t sjis | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0x82, 0xa0, 0x82, 0xe8, 0x82, 0xc1, 0x82, 0xbd, 0x82, 0xaf, 0x82, 0xcc,
                                      0x96, 0xb2, 0x82, 0xf0, 0x82, 0xa9, 0x82, 0xab, 0x8f, 0x57, 0x82, 0xdf };
    std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

    std::string threatPathUtf8 = "ありったけの夢をかき集め";
    std::string utf8Path = toUtf8(threatPath, false);
    EXPECT_EQ(utf8Path, threatPathUtf8);

    threatPathUtf8 += " (Shift-JIS)";
    utf8Path = toUtf8(threatPath, true);
    EXPECT_EQ(utf8Path, threatPathUtf8);

    utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPathUtf8);
}

TEST_F(TestStringUtils, TestToUtf8FromLatin1)
{
    // echo -n "Ä ö ü ß" | iconv -f utf-8 -t latin1 | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0xc4, 0x20, 0xf6, 0x20, 0xfc, 0x20, 0xdf };
    std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

    std::string threatPathUtf8 = "Ä ö ü ß";
    std::string utf8Path = toUtf8(threatPath, false);
    EXPECT_EQ(utf8Path, threatPathUtf8);

    threatPathUtf8 += " (Latin1)";
    utf8Path = toUtf8(threatPath, true);
    EXPECT_EQ(utf8Path, threatPathUtf8);

    utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPathUtf8);
}

TEST_F(TestStringUtils, TestFromLogLevelToStringReturnsExpectedString)
{
    EXPECT_EQ(fromLogLevelToString(log4cplus::OFF_LOG_LEVEL), "OFF");
    EXPECT_EQ(fromLogLevelToString(log4cplus::TRACE_LOG_LEVEL), "TRACE");
    EXPECT_EQ(fromLogLevelToString(log4cplus::DEBUG_LOG_LEVEL), "DEBUG");
    EXPECT_EQ(fromLogLevelToString(log4cplus::INFO_LOG_LEVEL), "INFO");
    EXPECT_EQ(fromLogLevelToString(log4cplus::SUPPORT_LOG_LEVEL), "SUPPORT");
    EXPECT_EQ(fromLogLevelToString(log4cplus::WARN_LOG_LEVEL), "WARN");
    EXPECT_EQ(fromLogLevelToString(log4cplus::ERROR_LOG_LEVEL), "ERROR");
    EXPECT_EQ(fromLogLevelToString(999), "Unknown (999)");
}

TEST_F(TestStringUtils, TestPluralize)
{
    EXPECT_EQ(common::pluralize(1, "single", "plural"), "single");
    EXPECT_EQ(common::pluralize(2, "single", "plural"), "plural");
    EXPECT_EQ(common::pluralize(99, "single", "plural"), "plural");
    EXPECT_EQ(common::pluralize(0, "single", "plural"), "plural");
}

TEST_F(TestStringUtils, testgetSuSiStyleTimestamp)
{
    const std::regex expected_regex("([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3}Z)");

    EXPECT_TRUE( std::regex_match(getSusiStyleTimestamp(), expected_regex));
}

TEST_F(TestStringUtils, escapePathForLogging)
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6"
                             " \016 \017 \020 \021 \022 \023 \024 \025 \026 \027"
                             " \030 \031 \032 \033 \034 \035 \036 \037"
                             " \177 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    auto escaped = escapePathForLogging(threatPath);
    EXPECT_EQ(escaped, "ありったけの夢をかき集め \\001 \\002 \\003 \\004 \\005 \\006"
                          " \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027"
                          " \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037"
                          " \\177 \\\\ Ἄνδρα μοι ἔννεπε \\a \\b \\t \\n \\v \\f \\r Ä Ö Ü ß");
}

TEST_F(TestStringUtils, pathForLoggingAddsSingleQuotes)
{
    std::string threatPath = "/tmp/A path with spaces that looks horrible in the logs";
    auto escapedAndSingleQuoted = pathForLogging(threatPath);
    EXPECT_EQ(escapedAndSingleQuoted, "'/tmp/A path with spaces that looks horrible in the logs'");

    threatPath = "";
    escapedAndSingleQuoted = pathForLogging(threatPath);
    EXPECT_EQ(escapedAndSingleQuoted, "''");
}

TEST_F(TestStringUtils, pathForLoggingAddsSingleQuotesAndEscapes)
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6"
                             " \016 \017 \020 \021 \022 \023 \024 \025 \026 \027"
                             " \030 \031 \032 \033 \034 \035 \036 \037"
                             " \177 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    auto escapedAndSingleQuoted = pathForLogging(threatPath);
    EXPECT_EQ(escapedAndSingleQuoted, "'ありったけの夢をかき集め \\001 \\002 \\003 \\004 \\005 \\006"
                       " \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027"
                       " \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037"
                       " \\177 \\\\ Ἄνδρα μοι ἔννεπε \\a \\b \\t \\n \\v \\f \\r Ä Ö Ü ß'");
}
