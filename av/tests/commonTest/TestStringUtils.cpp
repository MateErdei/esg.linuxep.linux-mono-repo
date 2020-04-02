/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include <common/StringUtils.h>


TEST(TestStringUtils, TestXMLEscapeEscapesControlCharacters) // NOLINT
{
    std::string threatPath = "abc \1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \\ abc \a \b \t \n \v \f \r abc";
    common::escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "abc \\1 \\2 \\3 \\4 \\5 \\6 \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027 \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037 \\177 \\\\ abc \\a \\b \\t \\n \\v \\f \\r abc");
}

TEST(TestStringUtils, TestXMLEscapeNotEscapingWeirdCharacters) // NOLINT
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037 \177 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    common::escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "ありったけの夢をかき集め \\1 \\2 \\3 \\4 \\5 \\6 \\016 \\017 \\020 \\021 \\022 \\023 \\024 \\025 \\026 \\027 \\030 \\031 \\032 \\033 \\034 \\035 \\036 \\037 \\177 \\\\ Ἄνδρα μοι ἔννεπε \\a \\b \\t \\n \\v \\f \\r Ä Ö Ü ß");
}

TEST(TestStringUtils, TestToUtf8) // NOLINT
{
    std::string threatPath = "abc  \1 \2 \3 \4 \5 \6 \\ efg \a \b \t \n \v \f \r hik";
    std::string utf8Path = common::toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPath);
}

TEST(TestStringUtils, TestToUtf8WeirdCharacters) // NOLINT
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    std::string utf8Path = common::toUtf8(threatPath);
    EXPECT_EQ(utf8Path, threatPath);
}
