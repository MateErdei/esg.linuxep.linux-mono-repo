/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/


#include "MCSHttpClient.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>

#include <memory>
namespace MCS
{
    MCSHttpClient::MCSHttpClient(std::string mcsUrl, std::string registerToken):
        m_base_url(mcsUrl),m_registerToken(registerToken){}

    Common::HttpRequests::Response  MCSHttpClient::sendMessage(
        const std::string& url,
        Common::HttpRequests::Headers headers = {},
        Common::HttpRequests::RequestType requestType = Common::HttpRequests::RequestType::GET)
    {
        Common::HttpRequests::Headers requestHeaders;
        requestHeaders.insert({"Authorization",getV1AuthorizationHeader()});
        requestHeaders.insert({"User-Agent", "Sophos MCS Client/1.0.0 Linux sessions "+ m_registerToken});
        for (const auto& head : headers)
        {
            requestHeaders.insert({head.first,head.second});
        }
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        Common::HttpRequests::RequestConfig request{ .url = m_base_url+ url ,.headers = requestHeaders,.requestType= requestType};
        if (!m_proxy.empty())
        {
            request.proxy=m_proxy;
        }
        if (!m_proxyUser.empty())
        {
            request.proxyUsername=m_proxyUser;
            request.proxyPassword=m_proxyPassword;
        }
        Common::HttpRequests::Response response = client->get(request);
        return response;
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessageWithID(
        const std::string& url,
        Common::HttpRequests::Headers headers = {},
        Common::HttpRequests::RequestType requestType = Common::HttpRequests::RequestType::GET)
    {
        return sendMessage(url + getID(),headers,requestType);
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessageWithIDAndRole(
        const std::string& url,
        Common::HttpRequests::Headers headers = {},
        Common::HttpRequests::RequestType requestType = Common::HttpRequests::RequestType::GET)
    {
        return sendMessage(url + getID() + "/role/endpoint",headers, requestType);
    }

    std::string MCSHttpClient::getV1AuthorizationHeader()
    {
        std::string tobeEncoded = getID() + ":" + getPassword();
        std::string header = "Basic " + tobeEncoded;
        return header;
    }

    std::string MCSHttpClient::getID()
    {
        return m_id;
    }
    std::string MCSHttpClient::getPassword()
    {
        return m_password;
    }

    void MCSHttpClient::setID(const std::string& id)
    {
        m_id = id;
    }

    void MCSHttpClient::setPassword(const std::string& password)
    {
        m_password = password;
    }
    void MCSHttpClient::setProxyInfo(const std::string& proxy,const std::string& proxyUser,const std::string& proxyPassword)
    {
        m_proxy = proxy;
        m_proxyUser = proxyUser;
        m_proxyPassword = proxyPassword;
    }

}