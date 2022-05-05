/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <string>

namespace Common::XmlUtilities
{
    class Validation
    {
    public:
        static bool stringWillNotBreakXmlParsing(const std::string& xml);
    };
} // namespace Common::XmlUtilities
