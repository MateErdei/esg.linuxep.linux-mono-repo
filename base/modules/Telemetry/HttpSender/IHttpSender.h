/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "../HttpSenderImpl/RequestConfig.h"

#include <memory>
#include <string>
#include <vector>

class IHttpSender
{
public:
    virtual ~IHttpSender() = default;

    virtual int httpsRequest(std::shared_ptr<RequestConfig> requestConfig) = 0;
};
