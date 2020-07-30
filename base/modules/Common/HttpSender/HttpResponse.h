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
        HttpResponse(int code): httpCode{code}{}
        HttpResponse(int code, std::string desc): httpCode(code),description(desc){}
        HttpResponse() = default; 
        int httpCode =0 ; 
        int exitCode = 0; 
        std::string description;
        std::string bodyContent; 
    };
}
