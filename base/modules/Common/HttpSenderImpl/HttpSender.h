/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
        HttpSender& operator= (const HttpSender&) = delete;
        ~HttpSender() override;

        int doHttpsRequest(std::shared_ptr<RequestConfig> requestConfig) override;
    private:
        curl_slist* setCurlOptions(
            CURL* curl,
            std::shared_ptr<RequestConfig> requestConfig);

        std::shared_ptr<Common::HttpSender::ICurlWrapper> m_curlWrapper;
    };
} // namespace Common::HttpSenderImpl
