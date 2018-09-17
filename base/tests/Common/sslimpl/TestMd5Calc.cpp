/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/sslimpl/Md5Calc.h>

#include "gtest/gtest.h"

using namespace Common::sslimpl;
using PairResult = std::pair<std::string , std::string >;
using ListInputOutput = std::vector<PairResult >;


TEST(TestMd5Calc, MD5ShouldProduceExcectedValue) // NOLINT
{
    ListInputOutput listInputOutput = {
            {"hello world!", "fc3ff98e8c6a0d3087d515c0473f8677"},
            {"this is a\nmultiline string", "ce9107bda89e91c8f277ace9056e1322"},
            {"", "d41d8cd98f00b204e9800998ecf8427e"}
    };

    for ( auto inputoutput : listInputOutput)
    {
        std::string  input = inputoutput.first;
        std::string  expected = inputoutput.second;
        EXPECT_EQ(md5(input), expected);
    }
}
