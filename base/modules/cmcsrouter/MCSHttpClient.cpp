/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/


#include "MCSHttpClient.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <vector>
#include <memory>
namespace MCS
{
    Common::HttpRequests::Response  MCSHttpClient::sendMessage(const std::string& url)
    {
        std::map<std::string,std::string> headers;
        headers.insert({"Authorization",getV1AuthorizationHeader()});

//        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
//            std::make_shared<Common::CurlWrapper::CurlWrapper>();
//        std::shared_ptr<Common::HttpRequests::IHttpRequester> client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper;
        Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
        Common::HttpRequests::Response response =
            client.get(Common::HttpRequests::RequestConfig{ .url = url ,.headers = headers});
        return response;
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessageWithID(const std::string& url)
    {
        return sendMessage(url+getID());
    }

    Common::HttpRequests::Response  MCSHttpClient::sendMessageWithIDAndRole(const std::string& url)
    {
        return sendMessage(url+getID() + "/role/endpoint");
    }

    std::string MCSHttpClient::getV1AuthorizationHeader()
    {
        std::string tobeEncoded = getID() + ":" + getPassword();
        std::string header = "Basic " + tobeEncoded;
        return header;
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

    void MCSHttpClient::setPassword(const std::string& password)
    {
        m_password = password;
    }

}