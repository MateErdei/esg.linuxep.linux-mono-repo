/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

#include <string>
#include <vector>

namespace Common::HttpSenderImpl
{
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

    static const char* GL_defaultServer = "t1.sophosupd.com";
    static const int GL_defaultPort = 443;
    static const ResourceRoot GL_defaultResourceRoot = ResourceRoot::PROD;

    class RequestConfig
    {
    public:
        RequestConfig(
            const std::string& requestTypeStr,
            std::vector<std::string> additionalHeaders,
            std::string server = GL_defaultServer,
            int port = GL_defaultPort,
            std::string certPath = Common::FileSystem::join(
                ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory(),
                "telemetry_cert.pem"),
            ResourceRoot resourceRoot = GL_defaultResourceRoot);
        RequestConfig(const RequestConfig&) = delete;
        RequestConfig& operator=(const RequestConfig&) = delete;
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
        int getPort();

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
} // namespace Common::HttpSenderImpl