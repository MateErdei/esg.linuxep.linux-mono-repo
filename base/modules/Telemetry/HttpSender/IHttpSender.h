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

    virtual void setServer(const std::string& server) = 0;
    virtual void setPort(const int& port) = 0;

    virtual int getRequest(const std::vector<std::string>& additionalHeaders) = 0;
    virtual int postRequest(const std::vector<std::string>& additionalHeaders,
                      const std::string& jsonStruct) = 0;
};
