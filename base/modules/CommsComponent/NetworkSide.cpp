/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NetworkSide.h"
#include "Logger.h"
#include <Common/HttpSenderImpl/CurlWrapper.h>

namespace{
    std::weak_ptr<CommsComponent::NetworkSide> GL_weakReferenceToNetWorkSide; 
}

namespace CommsComponent
{
    NetworkSide::NetworkSide():
        m_sender(std::make_unique<Common::HttpSenderImpl::HttpSender>(
            std::make_shared<Common::HttpSenderImpl::CurlWrapper>()) )
            {

            }
    std::shared_ptr<NetworkSide> NetworkSide::createOrShareNetworkSide()
    {
        if ( auto sharedReference = GL_weakReferenceToNetWorkSide.lock())
        {
            return sharedReference; 
        } 
        else
        {
            std::shared_ptr<NetworkSide> ref = std::shared_ptr<NetworkSide>(new NetworkSide());  
            GL_weakReferenceToNetWorkSide = ref; 
            return ref; 
        }
    }

    Common::HttpSender::HttpResponse NetworkSide::performRequest(const Common::HttpSender::RequestConfig& request)
    {
        LOGINFO("Perform " << request.getRequestTypeAsString()  << " request to " << request.getServer() ); 
        return m_sender->fetchHttpRequest(request, true);
    }
        
}