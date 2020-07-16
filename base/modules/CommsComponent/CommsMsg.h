/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include <variant>
#include <map>
namespace CommsComponent
{
    struct Body
    {
        std::string body; 
    };
    
    struct BodyFile
    {
        std::string path2File;
    };

    using MapValues = std::map<std::string, std::string>; 
    using BodyOption = std::variant<Body, BodyFile>;
    struct HttpRequest
    {
        std::string url;         
        enum class Method{ Get, Post}; 
        Method method = Method::Get; 
        BodyOption bodyOption; 
        int port = 443; 
        bool proxyAllowed = false; 
        int timeout= 0 ; 
        MapValues headers; 
        MapValues params; 
    };

    struct HttpResponse
    {
        int httpCode; 
        std::string description;
        BodyOption bodyOption; 
        MapValues header;  
    };
    
    struct CommsMsg
    {
        static CommsMsg fromString(const std::string & ); 
        static std::string serialize(const CommsMsg& ); 
        std::string id; 
        std::variant<HttpRequest, HttpResponse> content; 
    };    
}
