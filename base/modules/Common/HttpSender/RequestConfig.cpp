/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "RequestConfig.h"

#include <sstream>

namespace Common::HttpSender
{
    RequestConfig::RequestConfig(
        RequestType requestType,
        std::vector<std::string> additionalHeaders,
        const std::string& server,
        int port,
        const std::string& certPath,
        const std::string& resourcePath) :
        m_additionalHeaders(std::move(additionalHeaders)),
        m_server(std::move(server)),
        m_port(port),
        m_requestType(requestType),
        m_certPath(std::move(certPath)),
        m_resourcePath(resourcePath)
    {
    }

    std::string RequestConfig::requestTypeToString(RequestType requestType)
    {
        switch (requestType)
        {
            case RequestType::GET:
                return "GET";
            case RequestType::POST:
                return "POST";
            case RequestType::PUT:
                return "PUT";
            default:
                throw std::logic_error("Unknown request type");
        }
    }

    RequestType RequestConfig::stringToRequestType(const std::string& requestType)
    {
        if (requestType == "GET")
        {
            return RequestType::GET;
        }
        else if (requestType == "POST")
        {
            return RequestType::POST;
        }
        else if (requestType == "PUT")
        {
            return RequestType::PUT;
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown request type: " << requestType;
            throw std::range_error(ss.str());
        }
    }

    void RequestConfig::setData(const std::string& data) { m_data = data; }

    void RequestConfig::setServer(const std::string& server) { m_server = server; }
    void RequestConfig::setPort(const int port) { m_port = port; }

    void RequestConfig::setCertPath(const std::string& certPath) { m_certPath = certPath; }

    void RequestConfig::setResourcePath(const std::string& resourcePath) { m_resourcePath = resourcePath; }
    void RequestConfig::setAdditionalHeaders(std::vector<std::string> headers){ m_additionalHeaders = headers; }

    RequestType RequestConfig::getRequestType() const { return m_requestType; }

    const std::string& RequestConfig::getCertPath() const { return m_certPath; }

    const std::string& RequestConfig::getData() const { return m_data; }

    std::vector<std::string> RequestConfig::getAdditionalHeaders() const { return m_additionalHeaders; }

    const std::string& RequestConfig::getServer() const { return m_server; }

    std::string RequestConfig::getRequestTypeAsString() const { return requestTypeToString(m_requestType); }

    const std::string& RequestConfig::getResourcePath() const { return m_resourcePath; }

    int RequestConfig::getPort() const { return m_port; }
} // namespace Common::HttpSenderImpl