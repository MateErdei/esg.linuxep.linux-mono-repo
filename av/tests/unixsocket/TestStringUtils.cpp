/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "unixsocket/StringUtils.h"


using namespace unixsocket;

TEST(TestStringUtils, TestXMLEscape) // NOLINT
{
    std::string threatPath = "abc \1 \2 \3 \4 \5 \6 \\ abc \a \b \t \n \v \f \r abc";
    xmlEscape(threatPath);
    EXPECT_EQ(threatPath, "abc \\1 \\2 \\3 \\4 \\5 \\6 \\ abc \\a \\b \\t \\n \\v \\f \\r abc");
}