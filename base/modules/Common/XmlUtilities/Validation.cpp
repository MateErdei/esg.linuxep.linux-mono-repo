/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Validation.h"

#include <array>

namespace Common::XmlUtilities
{
    bool Validation::stringWillNotBreakXmlParsing(const std::string& xml)
    {
        if (xml.empty())
        {
            return false;
        }
        std::array<char, 5> invalidChars = { '<', '&', '>', '\'', '\"' };
        for (char invalidChar : invalidChars)
        {
            if (xml.find(invalidChar) != std::string::npos)
            {
                return false;
            }
        }
        return true;
    }
} // namespace Common::XmlUtilities