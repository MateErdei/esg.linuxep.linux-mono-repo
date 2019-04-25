/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CurlWrapper.h"

#include <Telemetry/HttpSender/IHttpSender.h>

#include <memory>

class HttpSender : public IHttpSender
{
public:
    explicit HttpSender(
        std::string server = "https://t1.sophosupd.com/",
        std::shared_ptr<ICurlWrapper> curlWrapper = std::make_shared<CurlWrapper>());
    HttpSender(const HttpSender&) = delete;
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
        const std::string& verb,
        const std::string& certPath,
        const std::vector<std::string>& additionalHeaders = std::vector<std::string>(),
        const std::string& data = std::string());

    std::string m_server;
    std::shared_ptr<ICurlWrapper> m_curlWrapper;
};
