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

    class RequestConfig
    {
    public:
        RequestConfig(
            RequestType requestType,
            std::vector<std::string> additionalHeaders,
            const std::string& server,
            int port,
            const std::string& certPath,
            const std::string& resourcePath);

        RequestConfig(const RequestConfig&) = delete;
        RequestConfig& operator=(const RequestConfig&) = delete;
        ~RequestConfig() = default;

        void setData(const std::string& data);
        void setServer(const std::string& server);
        void setCertPath(const std::string& certPath);
        void setResourcePath(const std::string& resourcePath);

        const std::string& getData();
        std::vector<std::string> getAdditionalHeaders();
        RequestType getRequestType();
        const std::string& getCertPath();
        const std::string& getServer();
        const std::string& getResourcePath();
        std::string getRequestTypeAsString();
        int getPort();

        static RequestType stringToRequestType(const std::string& requestType);
        static std::string requestTypeToString(RequestType requestType);

    private:
        std::string m_data;
        std::vector<std::string> m_additionalHeaders;

        std::string m_server;
        int m_port;
        RequestType m_requestType;
        std::string m_certPath;
        std::string m_resourcePath;
    };
} // namespace Common::HttpSenderImpl