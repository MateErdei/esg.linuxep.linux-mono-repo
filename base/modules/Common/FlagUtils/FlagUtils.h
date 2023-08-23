/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <optional>
#include <string>

namespace Common
{
    class FlagUtils
    {
    public:
        static bool isFlagSet(const std::string& flag, const std::string& flagContent);
    };
} // namespace Common