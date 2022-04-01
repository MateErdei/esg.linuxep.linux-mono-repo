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
        MCSHttpClient(std::string mcsUrl, std::string registerToken,std::shared_ptr<Common::HttpRequests::IHttpRequester>);
        Common::HttpRequests::Response  sendRegistration(Common::HttpRequests::Headers headers, const std::string& urlSuffix, const std::string& statusXml);
        Common::HttpRequests::Response  sendMessage(const std::string& url,Common::HttpRequests::RequestType requestType,Common::HttpRequests::Headers headers);
        Common::HttpRequests::Response  sendMessageWithID(const std::string& url,Common::HttpRequests::RequestType requestType,Common::HttpRequests::Headers headers);
        Common::HttpRequests::Response  sendMessageWithIDAndRole(const std::string& url,Common::HttpRequests::RequestType requestType,Common::HttpRequests::Headers headers);
        std::string getID();
        std::string getPassword();

        void setID(const std::string& id);
        void setVersion(const std::string& version);
        void setPassword(const std::string& password);
        void setCertPath(const std::string& password);
        void setProxyInfo(const std::string& proxy,const std::string& proxyUser,const std::string& proxyPassword);

    private:
        std::string getV1AuthorizationHeader();
        std::string m_base_url;
        std::string m_registerToken;
        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        std::string m_id;
        std::string m_version;
        std::string m_password;
        std::string m_proxy;
        std::string m_proxyUser;
        std::string m_proxyPassword;
        std::string m_certPath;

    };
}
