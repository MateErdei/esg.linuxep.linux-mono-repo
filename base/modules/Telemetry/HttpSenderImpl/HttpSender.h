/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CurlWrapper.h"

#include <Telemetry/HttpSender/IHttpSender.h>

#include <memory>

enum class RequestType
{
    GET,
    POST,
    PUT
};

class HttpSender : public IHttpSender
{
public:
    explicit HttpSender(
        std::string& server,
        std::shared_ptr<ICurlWrapper>& curlWrapper);
    HttpSender(const HttpSender&) = delete;
    HttpSender& operator= (const HttpSender&) = delete;
    ~HttpSender() override = default;

    void setServer(const std::string& server) override;

    int getRequest(const std::vector<std::string>& additionalHeaders, const std::string& certPath) override;
    int postRequest(
        const std::vector<std::string>& additionalHeaders,
        const std::string& data,
        const std::string& certPath) override;
    int putRequest(
        const std::vector<std::string>& additionalHeaders,
        const std::string& data,
        const std::string& certPath) override;

private:
    int httpsRequest(
        const RequestType& requestType,
        const std::string& certPath,
        const std::vector<std::string>& additionalHeaders = std::vector<std::string>(),
        const std::string& data = std::string());
    std::string requestTypeToString(RequestType requestType);

    std::string m_server;
    std::shared_ptr<ICurlWrapper> m_curlWrapper;
};
