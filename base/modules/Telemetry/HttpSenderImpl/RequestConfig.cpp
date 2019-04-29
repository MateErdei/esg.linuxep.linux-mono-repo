/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "RequestConfig.h"

#include <sstream>

RequestConfig::RequestConfig(
    const std::string& requestTypeStr,
    std::vector<std::string> additionalHeaders,
    std::string server,
    int port,
    std::string certPath,
    ResourceRoot resourceRoot )
: m_additionalHeaders(std::move(additionalHeaders))
, m_server(std::move(server))
, m_port(port)
, m_requestType(stringToRequestType(requestTypeStr))
, m_certPath(std::move(certPath))
, m_resourceRoot(resourceRoot)
{
}

std::string RequestConfig::resourceRootToString(ResourceRoot resourceRoot)
{
    switch(resourceRoot)
    {
        case ResourceRoot::PROD:
            return "/linux/sspl/prod";
        case ResourceRoot::DEV:
            return "/linux/sspl/dev";
        case ResourceRoot::TEST:
            return "";
        default:
            throw std::logic_error("Unknown resource root");
    }
}

ResourceRoot RequestConfig::stringToResourceRoot(const std::string& resourceRoot)
{
    if (resourceRoot == "PROD")
    {
        return ResourceRoot::PROD;
    }
    else if (resourceRoot == "DEV")
    {
        return ResourceRoot::DEV;
    }
    else if (resourceRoot == "TEST")
    {
        return ResourceRoot::TEST;
    }
    else
    {
        std::stringstream ss;
        ss << "Unknown resource root: " << resourceRoot;
        throw std::runtime_error(ss.str());
    }
}

std::string RequestConfig::requestTypeToString(RequestType requestType)
{
    switch(requestType)
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
        throw std::runtime_error(ss.str());
    }
}

void RequestConfig::setData(const std::string& data)
{
    m_data = data;
}

void RequestConfig::setServer(const std::string& server)
{
    m_server = server;
}

void RequestConfig::setCertPath(const std::string& certPath)
{
    m_certPath = certPath;
}

void RequestConfig::setResourceRoot(const std::string& resourceRoot)
{
    m_resourceRoot = stringToResourceRoot(resourceRoot);
}

RequestType RequestConfig::getRequestType()
{
    return m_requestType;
}

std::string RequestConfig::getCertPath()
{
    return m_certPath;
}

std::string RequestConfig::getData()
{
    return m_data;
}

std::vector<std::string> RequestConfig::getAdditionalHeaders()
{
    return m_additionalHeaders;
}

std::string RequestConfig::getServer()
{
    return m_server;
}

std::string RequestConfig::getRequestTypeAsString()
{
    return requestTypeToString(m_requestType);
}

std::string RequestConfig::getResourceRootAsString()
{
    return resourceRootToString(m_resourceRoot);
}