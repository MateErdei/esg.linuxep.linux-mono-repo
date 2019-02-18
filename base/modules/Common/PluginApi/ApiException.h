/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <exception>
#include <stdexcept>

namespace Common
{
    namespace PluginApi
    {
        /**
         * Exception class to report failures when handling the api requests.
         */
        class ApiException : public std::runtime_error
        {
        public:
            explicit ApiException(const std::string& what) : std::runtime_error(what) {}
        };
    } // namespace PluginApi
} // namespace Common
