/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <exception>
#include <stdexcept>

namespace Common
{
    namespace UtilityImpl
    {
        /**
         * Use this exception to indicate that a required configuration is wrong and there is no way to restore or run
         * the system without that configuration.
         */
        class ConfigException : public std::runtime_error
        {
        public:
            using std::runtime_error::runtime_error;
            ConfigException( const std::string&  where, const std::string&  cause);
        };
    }
}

