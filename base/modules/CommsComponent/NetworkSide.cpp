/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NetworkSide.h"
#include "Logger.h"
#include "CommsMsg.h"
#include <Common/HttpSenderImpl/CurlWrapper.h>

namespace
{
    std::weak_ptr<CommsComponent::NetworkSide> GL_weakReferenceToNetWorkSide;
}

namespace CommsComponent
{
    NetworkSide::NetworkSide() :
            m_sender(std::make_unique<Common::HttpSenderImpl::HttpSender>(
                    std::make_shared<Common::HttpSenderImpl::CurlWrapper>()))
    {

    }

    std::shared_ptr<NetworkSide> NetworkSide::createOrShareNetworkSide()
    {
        if (auto sharedReference = GL_weakReferenceToNetWorkSide.lock())
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
        LOGINFO("Perform " << request.getRequestTypeAsString() << " request to " << request.getServer());
        long curlCode;
        auto response = m_sender->fetchHttpRequest(request, m_proxy,  true, &curlCode);
        LOGDEBUG("Curl exit code: " << curlCode);
        return response;
    }

    void NetworkSide::setProxy(const Common::HttpSenderImpl::ProxySettings& proxy)
    {
        m_proxy = proxy; 
    }

    CommsNetwork::CommsNetwork():m_networkSide{ CommsComponent::NetworkSide::createOrShareNetworkSide()}
    {

    } 
    int CommsNetwork::operator()(std::shared_ptr<MessageChannel> channel, IOtherSideApi & parentProxy)
    {
        while(true)
        {
            std::string message; 
            channel->pop(message); 
            CommsMsg comms = CommsComponent::CommsMsg::fromString(message);
            if (std::holds_alternative<Common::HttpSender::RequestConfig>(comms.content))
            {
                auto response = m_networkSide->performRequest(std::get<Common::HttpSender::RequestConfig>(comms.content));
                comms.content = response; 
                parentProxy.pushMessage(CommsMsg::serialize(comms)); 
            }
            else if(std::holds_alternative<CommsComponent::CommsConfig>(comms.content))
            {
                auto config = std::get<CommsComponent::CommsConfig>(comms.content);
                Common::HttpSenderImpl::ProxySettings proxy; 
                proxy.proxy = config.getProxy(); 
                proxy.credentials = config.getDeobfuscatedCredential(); 
                m_networkSide->setProxy(proxy); 
            }

        }
        return 0; 
    }


}