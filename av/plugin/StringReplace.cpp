/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StringReplace.h"

namespace Example
{
    std::string orderedStringReplace(const std::string& pattern, const KeyValueCollection& keyvalues)
    {
        std::string result;
        size_t beginPos = 0;

        for (auto & keyvalue : keyvalues)
        {
            auto & key = keyvalue.first;
            size_t pos = pattern.find(key, beginPos);
            if (pos == std::string::npos)
            {
                break;
            }
            result += pattern.substr(beginPos, pos-beginPos);
            result += keyvalue.second;
            beginPos = pos + key.length();
        }

        result += pattern.substr(beginPos);
        return result;
    }
}