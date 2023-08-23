/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <string>
namespace OSUtilities
{
    class ISystemUtils
    {
    public:
        virtual ~ISystemUtils() = default;
        /***
         *
         * @param key name of the environment variable to get
         * @return value of the environment variable, defaulting to empty string
         */
        virtual std::string getEnvironmentVariable(const std::string& key) const = 0;
    };
} // namespace OSUtilities
