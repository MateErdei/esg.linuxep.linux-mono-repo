/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IHttpSender.h"

class HttpSender: public IHttpSender
{
public:
    HttpSender(const std::string& server, const int& port);
    HttpSender(const HttpSender&) = delete;
    ~HttpSender() override = default;

    void get_request(const std::vector<std::string>& additionalHeaders) override;

    void post_request(const std::vector<std::string>& additionalHeaders,
                           const std::string& jsonStruct) override;

private:
    void https_request(const std::string& verb,
                       const std::vector<std::string>& additionalHeaders = std::vector<std::string>(),
                       const std::string& jsonStruct = std::string());

    std::string m_server;
    int m_port;
};
