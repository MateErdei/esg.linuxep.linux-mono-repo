/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

#include <string>
#include <vector>

namespace Common::HttpSender
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

        RequestConfig() = default; 
        ~RequestConfig() = default;

        void setRequestTypeFromString(const std::string& ); 
        void setData(const std::string& data);
        void setServer(const std::string& server);
        void setPort(const int port);
        void setCertPath(const std::string& certPath);
        void setResourcePath(const std::string& resourcePath);
        void setAdditionalHeaders(std::vector<std::string>); 

        const std::string& getData() const;
        std::vector<std::string> getAdditionalHeaders() const;
        RequestType getRequestType() const;
        const std::string& getCertPath() const;
        const std::string& getServer() const;
        const std::string& getResourcePath() const ;
        std::string getRequestTypeAsString() const;
        int getPort() const;

        static RequestType stringToRequestType(const std::string& requestType);
        static std::string requestTypeToString(RequestType requestType);

    private:
        std::string m_data;
        std::vector<std::string> m_additionalHeaders;

        std::string m_server;
        int m_port = 0;
        RequestType m_requestType = RequestType::GET;
        std::string m_certPath;
        std::string m_resourcePath;
    };
} // namespace Common::HttpSenderImpl