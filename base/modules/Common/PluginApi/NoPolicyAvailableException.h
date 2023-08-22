/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ApiException.h"

namespace Common::PluginApi
{
    /**
     * Exception class to report that No Policy is available
     */
    class NoPolicyAvailableException : public ApiException
    {
    public:
        static const std::string NoPolicyAvailable;
        explicit NoPolicyAvailableException() : ApiException(NoPolicyAvailable) {}
    };
} // namespace Common::PluginApi
