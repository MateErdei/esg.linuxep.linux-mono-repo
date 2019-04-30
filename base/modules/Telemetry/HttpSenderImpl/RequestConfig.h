/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

enum class RequestType
{
    GET,
    POST,
    PUT
};

enum class ResourceRoot
{
    PROD,
    DEV,
    TEST
};

#define DEFAULT_SERVER "t1.sophosupd.com"
#define DEFAULT_CERT_PATH "/opt/sophos-spl/base/etc/sophosspl/telemetry_cert.pem"

constexpr static int g_defaultPort = 443;
constexpr static ResourceRoot g_defaultResourceRoot = ResourceRoot::PROD;


class RequestConfig
{
public:
    RequestConfig(
        const std::string& requestTypeStr,
        std::vector<std::string> additionalHeaders,
        std::string server = DEFAULT_SERVER,
        int port = g_defaultPort,
        std::string certPath = DEFAULT_CERT_PATH,
        ResourceRoot resourceRoot = g_defaultResourceRoot
        );
    RequestConfig(const RequestConfig&) = delete;
    RequestConfig& operator= (const RequestConfig&) = delete;
    ~RequestConfig() = default;

    void setData(const std::string& data);
    void setServer(const std::string& server);
    void setCertPath(const std::string& certPath);
    void setResourceRoot(const std::string& resourceRoot);

    const std::string& getData();
    std::vector<std::string> getAdditionalHeaders();
    RequestType getRequestType();
    const std::string& getCertPath();
    std::string getServer();
    std::string getResourceRootAsString();
    std::string getRequestTypeAsString();

    static std::string resourceRootToString(ResourceRoot resourceRoot);
    static std::string requestTypeToString(RequestType requestType);

private:
    std::string m_data;
    std::vector<std::string> m_additionalHeaders;
    static ResourceRoot stringToResourceRoot(const std::string& resourceRoot);
    static RequestType stringToRequestType(const std::string& requestType);

    std::string m_server;
    int m_port;
    RequestType m_requestType;
    std::string m_certPath;
    ResourceRoot m_resourceRoot;
};