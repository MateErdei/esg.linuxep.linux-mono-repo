/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/


#include "MCSHttpClient.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <Common/ObfuscationImpl/Base64.h>

#include <memory>
namespace MCS
{
    MCSHttpClient::MCSHttpClient(std::string mcsUrl, std::string registerToken,std::shared_ptr<Common::HttpRequests::IHttpRequester> client):
        m_base_url(mcsUrl),m_registerToken(registerToken),m_client(client){}

    Common::HttpRequests::Response  MCSHttpClient::sendRegistration(const Common::HttpRequests::Headers& headers, const std::string& urlSuffix, const std::string& statusXml)
    {
        Common::HttpRequests::Headers requestHeaders;

        Common::HttpRequests::Response response;
        requestHeaders.insert({"User-Agent", "Sophos MCS Client/" + m_version + " Linux sessions "+ m_registerToken});
        for (const auto& head : headers)
        {
            requestHeaders.insert({head.first,head.second});
        }

        Common::HttpRequests::RequestConfig request{ .url = m_base_url + urlSuffix, .headers = requestHeaders, .data = statusXml};
        if (!m_proxy.empty())
        {
            request.proxy = m_proxy;
            if (!m_proxyUser.empty())
            {
                request.proxyUsername = m_proxyUser;
                request.proxyPassword = m_proxyPassword;
            }
        }
        if (!m_certPath.empty())
        {
            request.certPath = m_certPath;
        }

        return m_client->post(request);
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessage(
        const std::string& url,
        Common::HttpRequests::RequestType requestType,
        Common::HttpRequests::Headers headers = {}
    )
    {
        Common::HttpRequests::Headers requestHeaders;
        requestHeaders.insert({"Authorization",getV1AuthorizationHeader()});
        requestHeaders.insert({"User-Agent", "Sophos MCS Client/" + m_version + " Linux sessions "+ m_registerToken});
        for (const auto& head : headers)
        {
            requestHeaders.insert({head.first,head.second});
        }

        Common::HttpRequests::RequestConfig request{ .url = m_base_url + url ,.headers = requestHeaders};
        if (!m_proxy.empty())
        {
            request.proxy = m_proxy;
            if (!m_proxyUser.empty())
            {
                request.proxyUsername = m_proxyUser;
                request.proxyPassword = m_proxyPassword;
            }
        }

        if (!m_certPath.empty())
        {
            request.certPath = m_certPath;
        }

        Common::HttpRequests::Response response;
        switch (requestType)
        {
            case Common::HttpRequests::RequestType::POST:
                response = m_client->post(request);
                break;
            case Common::HttpRequests::RequestType::PUT:
                response = m_client->put(request);
                break;
            case Common::HttpRequests::RequestType::GET:
                response = m_client->get(request);
                break;
            default :
                response = m_client->get(request);
                break;
        }
        return response;
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessageWithID(
        const std::string& url,
        Common::HttpRequests::RequestType requestType,
        Common::HttpRequests::Headers headers = {})
    {
        return sendMessage(url + getID(),requestType, headers);
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessageWithIDAndRole(
        const std::string& url,
        Common::HttpRequests::RequestType requestType,
        Common::HttpRequests::Headers headers = {})
    {
        return sendMessage(url + getID() + "/role/endpoint", requestType, headers);
    }

    std::string MCSHttpClient::getV1AuthorizationHeader()
    {
        std::string tobeEncoded = getID() + ":" + getPassword();
        std::string header = "Basic " + Common::ObfuscationImpl::Base64::Encode(tobeEncoded);
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

    void MCSHttpClient::setVersion(const std::string& version)
    {
        m_version = version;
    }

    void MCSHttpClient::setPassword(const std::string& password)
    {
        m_password = password;
    }

    void MCSHttpClient::setCertPath(const std::string& certPath)
    {
        m_certPath = certPath;
    }

    void MCSHttpClient::setProxyInfo(const std::string& proxy,const std::string& proxyUser,const std::string& proxyPassword)
    {
        m_proxy = proxy;
        m_proxyUser = proxyUser;
        m_proxyPassword = proxyPassword;
    }
}