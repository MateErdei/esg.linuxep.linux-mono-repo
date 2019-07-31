/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/HttpSenderImpl/RequestConfig.h>

#include <memory>
#include <string>
#include <vector>

namespace Common::HttpSender
{
    class IHttpSender
    {
    public:
        virtual ~IHttpSender() = default;

        virtual int doHttpsRequest(Common::HttpSenderImpl::RequestConfig& requestConfig) = 0;
    };
} // namespace Common::HttpSender
