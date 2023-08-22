/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ApiException.h"

namespace Common::PluginApi
{
    /**
     * Exception class to report that ACK has not been sent from the replier to the requester.
     */
    class NoACKReplyException : public ApiException
    {
    public:
        using ApiException::ApiException;
    };
} // namespace Common::PluginApi
