/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace SulDownloader
{
    struct Tag
    {
        Tag(std::string t, std::string b, std::string l)
                : tag(std::move(t)), baseversion(std::move(b)), label(std::move(l))
        {
        }

        std::string tag;
        std::string baseversion;
        std::string label;
    };
}


