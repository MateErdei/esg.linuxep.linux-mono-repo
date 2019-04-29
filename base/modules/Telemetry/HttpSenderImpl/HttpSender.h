/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CurlWrapper.h"

#include <Telemetry/HttpSender/IHttpSender.h>

class HttpSender : public IHttpSender
{
public:
    explicit HttpSender(std::shared_ptr<ICurlWrapper> curlWrapper);
    HttpSender(const HttpSender&) = delete;
    HttpSender& operator= (const HttpSender&) = delete;
    ~HttpSender() override = default;

    int httpsRequest(std::shared_ptr<RequestConfig> requestConfig) override;
private:
    curl_slist* setCurlOptions(
        CURL* curl,
        const std::shared_ptr<RequestConfig>& requestConfig);

    std::shared_ptr<ICurlWrapper> m_curlWrapper;
};
