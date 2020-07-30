/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include <HttpSender/HttpResponse.h>
#include <Common/HttpSender/RequestConfig.h>
#include <chrono>
/**
 * The client side api. This is the main class the clients of the commns component will use in order to send POST/PUT requests 
 *  or to issue a GET request. 
 * */
namespace CommsComponent
{

    class HttpRequesterException : public std::runtime_error
    {
    public:
        explicit HttpRequesterException(const std::string& what) : std::runtime_error(what) {}
    };

    class HttpRequester
    {
        public: 

        static Common::HttpSender::HttpResponse
        triggerRequest(const std::string &requesterName, const Common::HttpSender::RequestConfig& request, std::chrono::milliseconds timeout);

        static Common::HttpSender::HttpResponse
        triggerRequest(const std::string &requesterName, Common::HttpSender::RequestConfig&& request, std::string&& body, std::chrono::milliseconds timeout)
        {
            request.setData(body); 
            return triggerRequest(requesterName, request, timeout); 
        }


        static std::string generateId(const std::string& requesterName); 
    };
}