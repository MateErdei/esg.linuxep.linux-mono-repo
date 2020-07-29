/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include <HttpSender/HttpResponse.h>
#include <Common/HttpSender/RequestConfig.h>
/**
 * The client side api. This is the main class the clients of the commns component will use in order to send POST/PUT requests 
 *  or to issue a GET request. 
 * */
namespace CommsComponent
{
    class HttpRequester
    {
        public: 
        static Common::HttpSender::HttpResponse triggerRequest(const std::string& requesterName, Common::HttpSender::RequestConfig&& , std::string && body);
        static std::string generateId(const std::string& requesterName); 
    };
}