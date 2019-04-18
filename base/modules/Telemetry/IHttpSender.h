/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

class IHttpSender
{
public:
    virtual ~IHttpSender() = default;

    virtual void get_request(const std::vector<std::string>& additionalHeaders) = 0;

    virtual void post_request(const std::vector<std::string>& additionalHeaders,
                      const std::string& jsonStruct) = 0;
};
