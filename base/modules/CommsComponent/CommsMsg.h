/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include <variant>
#include <map>
#include <HttpSender/RequestConfig.h>
#include <HttpSender/HttpResponse.h>
namespace CommsComponent
{    
    struct CommsMsg
    {
        static CommsMsg fromString(const std::string & ); 
        static std::string serialize(const CommsMsg& ); 
        std::string id; 
        std::variant<Common::HttpSender::RequestConfig, Common::HttpSender::HttpResponse> content; 
    };    
}
