/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CurlWrapper.h"

#include <Telemetry/HttpSender/IHttpSender.h>

#include <memory>

#define HTTP_PORT 443

class HttpSender: public IHttpSender
{
public:
    explicit HttpSender(std::string server = "https://t1.sophosupd.com/", int port = HTTP_PORT, std::shared_ptr<ICurlWrapper> curlWrapper = std::make_shared<CurlWrapper>());
    HttpSender(const HttpSender&) = delete;
    ~HttpSender() override = default;

    void setServer(const std::string& server) override;
    void setPort(const int& port) override;

    int getRequest(const std::vector<std::string>& additionalHeaders,
                   const std::string& certPath) override;

    int postRequest(const std::vector<std::string>& additionalHeaders,
                    const std::string& jsonStruct,
                    const std::string& certPath) override;

    int putRequest(const std::vector<std::string>& additionalHeaders,
                   const std::string& jsonStruct,
                   const std::string& certPath) override;

private:
    int httpsRequest(const std::string& verb,
                     const std::string& certPath,
                     const std::vector<std::string>& additionalHeaders = std::vector<std::string>(),
                     const std::string& jsonStruct = std::string());
    std::string m_server;
    int m_port;
    std::shared_ptr<ICurlWrapper> m_curlWrapper;
};
