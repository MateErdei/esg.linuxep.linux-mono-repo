/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once


#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
namespace MCS
{
    class MCSHttpClient
    {
    public:
        MCSHttpClient(std::string mcsUrl, std::string registerToken);
        Common::HttpRequests::Response  sendMessage(const std::string& url,Common::HttpRequests::RequestType requestType,Common::HttpRequests::Headers headers);
        Common::HttpRequests::Response  sendMessageWithID(const std::string& url,Common::HttpRequests::RequestType requestType,Common::HttpRequests::Headers headers);
        Common::HttpRequests::Response  sendMessageWithIDAndRole(const std::string& url,Common::HttpRequests::RequestType requestType,Common::HttpRequests::Headers headers);
        std::string getID();
        std::string getPassword();

        void setID(const std::string& id);
        void setPassword(const std::string& password);
        void setCertPath(const std::string& password);
        void setProxyInfo(const std::string& proxy,const std::string& proxyUser,const std::string& proxyPassword);

    private:
        std::string getV1AuthorizationHeader();
        std::string m_base_url;
        std::string m_registerToken;
        std::string m_id;
        std::string m_password;
        std::string m_proxy;
        std::string m_proxyUser;
        std::string m_proxyPassword;
        std::string m_certPath;

    };
}
