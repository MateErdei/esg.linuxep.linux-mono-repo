/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * WARNING: There should only ever be a single instance of this class as it initialises libcurl on construction.
 * TODO: Convert class to a singleton or use a factory where libcurl initilisation can be done before instance creation.
 */

#pragma once

#include "CurlWrapper.h"

#include <Common/HttpSender/IHttpSender.h>

namespace Common::HttpSenderImpl
{
    class HttpSender : public Common::HttpSender::IHttpSender
    {
    public:
        explicit HttpSender(std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper);
        HttpSender(const HttpSender&) = delete;
        HttpSender& operator=(const HttpSender&) = delete;
        ~HttpSender() override;

        int doHttpsRequest(RequestConfig& requestConfig) override;

    private:
        CURLcode setCurlOptions(
            CURL* curl,
            RequestConfig& requestConfig);

        std::shared_ptr<Common::HttpSender::ICurlWrapper> m_curlWrapper;
    };
} // namespace Common::HttpSenderImpl
