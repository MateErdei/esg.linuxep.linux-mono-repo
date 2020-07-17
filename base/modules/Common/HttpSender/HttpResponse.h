/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include <vector>

namespace Common::HttpSender
{
    struct HttpResponse
    {
        int httpCode; 
        std::string description;
        std::string bodyContent; 
    };
}
