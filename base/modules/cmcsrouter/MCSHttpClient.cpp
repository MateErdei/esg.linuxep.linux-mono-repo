// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MCSHttpClient.h"

#include "Common/HttpRequests/IHttpRequester.h"
#include "Common/ObfuscationImpl/Base64.h"

#include <memory>
#include <utility>

namespace MCS
{
    MCSHttpClient::MCSHttpClient(
        std::string mcsUrl,
        std::string registerToken,
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client) :
        m_base_url(std::move(mcsUrl)), m_registerToken(std::move(registerToken)), m_client(std::move(client))
    {
    }

    Common::HttpRequests::Response MCSHttpClient::sendRegistration(
        const std::string& statusXml,
        const std::string& token)
    {
        Common::HttpRequests::Headers requestHeaders{ { "Authorization", getRegistrationAuthorizationHeader(token) },
                                                      { "User-Agent",
                                                        "Sophos MCS Client/" + m_version + " Linux sessions " + token },
                                                      { "Content-Type", "application/xml; charset=utf-8" } };

        Common::HttpRequests::RequestConfig request{
            .url = m_base_url + "/register", .headers = requestHeaders, .data = statusXml, .timeout = HTTP_MAX_TIMEOUT
        };
        updateProxyInfo(request);
        updateCertPath(request);

        return m_client->post(request);
    }

    Common::HttpRequests::Response MCSHttpClient::sendPreregistration(
        const std::string& statusXml,
        const std::string& customerToken)
    {
        Common::HttpRequests::Headers requestHeaders{
            { "Authorization", getDeploymentInfoV2AuthorizationHeader(customerToken) },
            { "User-Agent", "Sophos MCS Client/" + m_version + " Linux sessions " + customerToken },
            { "Content-Type", "application/json;charset=UTF-8" }
        };

        Common::HttpRequests::RequestConfig request{ .url = m_base_url + "/install/deployment-info/2",
                                                     .headers = requestHeaders,
                                                     .data = statusXml,
                                                     .timeout = HTTP_MAX_TIMEOUT };
        updateProxyInfo(request);
        updateCertPath(request);

        return m_client->post(request);
    }

    Common::HttpRequests::Response MCSHttpClient::sendMessage(
        const std::string& url,
        Common::HttpRequests::RequestType requestType,
        const Common::HttpRequests::Headers& headers)
    {
        Common::HttpRequests::Headers requestHeaders;
        requestHeaders.insert({ "Authorization", getV1AuthorizationHeader() });
        requestHeaders.insert(
            { "User-Agent", "Sophos MCS Client/" + m_version + " Linux sessions " + m_registerToken });
        for (const auto& head : headers)
        {
            requestHeaders.insert({ head.first, head.second });
        }

        Common::HttpRequests::RequestConfig request{ .url = m_base_url + url,
                                                     .headers = requestHeaders,
                                                     .timeout = HTTP_MAX_TIMEOUT };
        updateProxyInfo(request);
        updateCertPath(request);

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
            default:
                response = m_client->get(request);
                break;
        }
        return response;
    }

    Common::HttpRequests::Response MCSHttpClient::sendMessageWithID(
        const std::string& url,
        Common::HttpRequests::RequestType requestType,
        const Common::HttpRequests::Headers& headers)
    {
        return sendMessage(url + getID(), requestType, headers);
    }

    Common::HttpRequests::Response MCSHttpClient::sendMessageWithIDAndRole(
        const std::string& url,
        Common::HttpRequests::RequestType requestType,
        const Common::HttpRequests::Headers& headers)
    {
        return sendMessage(url + getID() + "/role/endpoint", requestType, headers);
    }

    std::string MCSHttpClient::getV1AuthorizationHeader()
    {
        return getAuthorizationHeader(getID() + ":" + getPassword());
    }

    std::string MCSHttpClient::getRegistrationAuthorizationHeader(const std::string& token)
    {
        return getAuthorizationHeader(getID() + ":" + getPassword() + ":" + token);
    }

    std::string MCSHttpClient::getDeploymentInfoV2AuthorizationHeader(const std::string& customerToken)
    {
        return getAuthorizationHeader(customerToken);
    }

    std::string MCSHttpClient::getAuthorizationHeader(const std::string& toBeEncoded)
    {
        return "Basic " + Common::ObfuscationImpl::Base64::Encode(toBeEncoded);
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

    void MCSHttpClient::setProxyInfo(
        const std::string& proxy,
        const std::string& proxyUser,
        const std::string& proxyPassword)
    {
        m_proxy = proxy;
        m_proxyUser = proxyUser;
        m_proxyPassword = proxyPassword;
    }

    void MCSHttpClient::updateProxyInfo(Common::HttpRequests::RequestConfig& request)
    {
        if (!m_proxy.empty())
        {
            request.proxy = m_proxy;
            if (!m_proxyUser.empty())
            {
                request.proxyUsername = m_proxyUser;
                request.proxyPassword = m_proxyPassword;
            }
        }
    }

    void MCSHttpClient::updateCertPath(Common::HttpRequests::RequestConfig& request)
    {
        if (!m_certPath.empty())
        {
            request.certPath = m_certPath;
        }
    }
} // namespace MCS