/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ApiException.h"


namespace Common
{
    namespace PluginApi
    {
        /**
         * Exception class to report that No Policy is available
         */
        class NoPolicyAvailableException : public ApiException
        {
        public:
            explicit NoPolicyAvailableException(const std::string& what) : ApiException(what) {}
        };
    } // namespace PluginApi
} // namespace Common
