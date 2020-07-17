/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "RequestConfig.h"

#include <memory>
#include <string>
#include <vector>

namespace Common::HttpSender
{
    class IHttpSender
    {
    public:
        virtual ~IHttpSender() = default;

        virtual int doHttpsRequest(RequestConfig& requestConfig) = 0;
    };
} // namespace Common::HttpSender
