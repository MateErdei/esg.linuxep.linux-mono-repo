/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <Common/HttpSender/RequestConfig.h>
#include <Common/HttpSender/HttpResponse.h>
#include <Common/HttpSenderImpl/HttpSender.h>
#include <memory>

namespace CommsComponent
{
    /** It is only safe to have a single Networkside running in your application*/
    class NetworkSide{
        NetworkSide(const NetworkSide&) = delete; 
        NetworkSide& operator=(const NetworkSide&) = delete; 
        NetworkSide(); 
        std::unique_ptr<Common::HttpSenderImpl::HttpSender> m_sender; 
        public:
        static std::shared_ptr<NetworkSide> createOrShareNetworkSide(); 

        Common::HttpSender::HttpResponse performRequest(const Common::HttpSender::RequestConfig & request); 
        
    };
}