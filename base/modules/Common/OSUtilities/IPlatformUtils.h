/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace OSUtilities
    {
        class IPlatformUtils
        {
        public:
            virtual ~IPlatformUtils() = default;

            virtual std::string getHostname() const = 0;
            virtual std::string getPlatform() const = 0;
            virtual std::string getVendor() const = 0;
            virtual std::string getArchitecture() const = 0;
        };



    } // namespace OSUtilities
} // namespace Common