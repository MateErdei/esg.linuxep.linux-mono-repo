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
    class InvalidCommsMsgException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    struct CommsMsg
    {
        static CommsMsg fromString(const std::string & ); 
        static std::string serialize(const CommsMsg& ); 
        std::string id; 
        std::variant<Common::HttpSender::RequestConfig, Common::HttpSender::HttpResponse> content; 
    };    
}
