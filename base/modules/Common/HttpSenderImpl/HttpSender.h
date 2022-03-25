/******************************************************************************************************

Copyright 2019-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * WARNING: There should only ever be a single instance of this class as it initialises libcurl on construction.
 * TODO: Convert class to a singleton or use a factory where libcurl initialisation can be done before instance creation.
 */

#pragma once


#include <Common/HttpSender/HttpResponse.h>
#include <Common/HttpSender/IHttpSender.h>
#include "CurlWrapper/ICurlWrapper.h"

namespace Common::HttpSenderImpl
{
    struct ProxySettings
    {
        // should contain the server and the port.
        // credentials should be user:password
        std::string proxy;
        std::string credentials;
    };

    class HttpSender : public Common::HttpSender::IHttpSender
    {
    public:
        explicit HttpSender(std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper);
        HttpSender(const HttpSender&) = delete;
        HttpSender& operator=(const HttpSender&) = delete;
        ~HttpSender() override;

        int doHttpsRequest(const Common::HttpSender::RequestConfig& requestConfig) override;
        Common::HttpSender::HttpResponse fetchHttpRequest(
            const Common::HttpSender::RequestConfig& requestConfig,
            const ProxySettings&,
            bool captureBody,
            long* curlCode);
        Common::HttpSender::HttpResponse fetchHttpRequest(
            const Common::HttpSender::RequestConfig& requestConfig,
            bool captureBody,
            long* curlCode);

    private:
        Common::HttpSender::HttpResponse doFetchHttpRequest(
            const Common::HttpSender::RequestConfig& requestConfig,
            const ProxySettings&,
            bool captureBody,
            long* curlCode);
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> m_curlWrapper;
    };
} // namespace Common::HttpSenderImpl
