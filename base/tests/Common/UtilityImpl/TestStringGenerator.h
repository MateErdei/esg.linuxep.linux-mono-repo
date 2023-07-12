// Copyright 2023 Sophos All rights reserved.

#pragma once

#include <string>
#include <vector>

namespace {
    static std::string generateNonUTF8String()
    {
        // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t euc-jp | hexdump -C
        std::vector<unsigned char> threatPathBytes { 0xa4, 0xa2, 0xa4, 0xea, 0xa4, 0xc3, 0xa4, 0xbf, 0xa4, 0xb1, 0xa4, 0xce,
                                                    0xcc, 0xb4, 0xa4, 0xf2, 0xa4, 0xab, 0xa4, 0xad, 0xbd, 0xb8, 0xa4, 0xe1 };
        std::string nonUtf8Str(threatPathBytes.begin(), threatPathBytes.end());
        return nonUtf8Str;
    }
}