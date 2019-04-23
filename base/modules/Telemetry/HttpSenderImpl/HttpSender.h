/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CurlWrapper.h"

#include <Telemetry/HttpSender/IHttpSender.h>

#include <memory>

class HttpSender: public IHttpSender
{
public:
    HttpSender(std::string server = "https://t1.sophosupd.com/", int port = 443, std::shared_ptr<ICurlWrapper> curlWrapper = std::make_shared<CurlWrapper>());
    HttpSender(const HttpSender&) = delete;
    ~HttpSender() override = default;

    void setServer(const std::string& server) override;
    void setPort(const int& port) override;

    int getRequest(const std::vector<std::string>& additionalHeaders) override;
    int postRequest(const std::vector<std::string>& additionalHeaders,
                           const std::string& jsonStruct) override;

private:
    int httpsRequest(const std::string& verb,
                       const std::vector<std::string>& additionalHeaders = std::vector<std::string>(),
                       const std::string& jsonStruct = std::string());

    std::string m_server;
    int m_port;
    std::shared_ptr<ICurlWrapper> m_curlWrapper;
};
