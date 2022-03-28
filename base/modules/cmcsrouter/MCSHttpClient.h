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
        Common::HttpRequests::Response  sendMessage(const std::string& url);
        Common::HttpRequests::Response  sendMessageWithID(const std::string& url);
        Common::HttpRequests::Response  sendMessageWithIDAndRole(const std::string& url);
        std::string getID();
        std::string getPassword();
        void setID(const std::string& id);
        void setPassword(const std::string& id);

    private:
        std::string getV1AuthorizationHeader();
        std::string m_id;
        std::string m_password;

    };
}
