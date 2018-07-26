/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace SulDownloader
{
    struct Tag
    {
        Tag(const std::string &t, const std::string &b, const std::string &l)
                : tag(t), baseversion(b), label(l)
        {
        }

        std::string tag;
        std::string baseversion;
        std::string label;
    };
}


