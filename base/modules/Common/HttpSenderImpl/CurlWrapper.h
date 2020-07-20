/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/HttpSender/ICurlWrapper.h>

namespace Common::HttpSenderImpl
{
    class CurlWrapper : public Common::HttpSender::ICurlWrapper
    {
    public:
        CurlWrapper() = default;
        CurlWrapper(const CurlWrapper&) = delete;
        CurlWrapper& operator=(const CurlWrapper&) = delete;
        ~CurlWrapper() override = default;

        CURLcode curlGlobalInit(long flags) override;
        CURL* curlEasyInit() override;

        CURLcode curlEasySetOptHeaders(CURL* handle, curl_slist* headers) override;
        CURLcode curlEasySetOpt(CURL* handle, CURLoption option, const std::variant<std::string, long> parameter) override;
        CURLcode curlGetResponseCode(CURL *handle, long* codep) override;

        curl_slist* curlSlistAppend(curl_slist* list, const std::string& value) override;

        CURLcode curlEasyPerform(CURL* handle) override;

        void curlSlistFreeAll(curl_slist* list) override;

        void curlEasyCleanup(CURL* handle) override;
        void curlGlobalCleanup() override;

        const char* curlEasyStrError(CURLcode errornum) override;
    };
} // namespace Common::HttpSenderImpl
