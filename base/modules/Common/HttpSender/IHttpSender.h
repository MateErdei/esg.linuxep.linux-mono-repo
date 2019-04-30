/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/HttpSenderImpl/RequestConfig.h>

#include <memory>
#include <string>
#include <vector>

namespace Common
{
    namespace HttpSender
    {
        class IHttpSender
        {
        public:
            virtual ~IHttpSender() = default;

            virtual int doHttpsRequest(std::shared_ptr<RequestConfig> requestConfig) = 0;
        };
    } // namespace HttpSender
} // namespace Common
