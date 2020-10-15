/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include <common/StringUtils.h>
#include <log4cplus/loglevel.h>
#include <Common/Logging/SophosLoggerMacros.h>

using namespace common;

TEST(TestStringUtils, TestXMLEscapeEscapesControlCharacters) // NOLINT
{
    std::string threatPath = "abc \1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \\ abc \a \b \t \n \v \f \r abc";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "abc \\1 \\2 \\3 \\4 \\5 \\6 \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027 \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037 \\177 \\\\ abc \\a \\b \\t \\n \\v \\f \\r abc");
}

TEST(TestStringUtils, TestXMLEscapeNotEscapingWeirdCharacters) // NOLINT
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "ありったけの夢をかき集め \\1 \\2 \\3 \\4 \\5 \\6 \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027 \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037 \\177 \\\\ Ἄνδρα μοι ἔννεπε \\a \\b \\t \\n \\v \\f \\r Ä Ö Ü ß");
}

TEST(TestStringUtils, TestXMLEscapeEscapesSpecialXMLCharacters) // NOLINT
{
    std::string threatPath = "abc & < > \' \" abc";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "abc &amp; &lt; &gt; &apos; &quot; abc");
}

TEST(TestStringUtils, TestToUtf8) // NOLINT
{
    std::string threatPath = "abc  \1 \2 \3 \4 \5 \6 \\ efg \a \b \t \n \v \f \r hik";
    std::string utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPath);
}

TEST(TestStringUtils, TestToUtf8WeirdCharacters) // NOLINT
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    std::string utf8Path = toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPath);
}

TEST(TestStringUtils, TestfromLogLevelToStringReturnsExpectedString)
{
    EXPECT_EQ(fromLogLevelToString(log4cplus::OFF_LOG_LEVEL), "OFF");
    EXPECT_EQ(fromLogLevelToString(log4cplus::DEBUG_LOG_LEVEL), "DEBUG");
    EXPECT_EQ(fromLogLevelToString(log4cplus::INFO_LOG_LEVEL), "INFO");
    EXPECT_EQ(fromLogLevelToString(log4cplus::SUPPORT_LOG_LEVEL), "SUPPORT");
    EXPECT_EQ(fromLogLevelToString(log4cplus::WARN_LOG_LEVEL), "WARN");
    EXPECT_EQ(fromLogLevelToString(log4cplus::ERROR_LOG_LEVEL), "ERROR");
}

TEST(TestStringUtils, Testpluralize)
{
    EXPECT_EQ(common::pluralize(1, "single", "plural"), "single");
    EXPECT_EQ(common::pluralize(2, "single", "plural"), "plural");
    EXPECT_EQ(common::pluralize(0, "single", "plural"), "plural");
}