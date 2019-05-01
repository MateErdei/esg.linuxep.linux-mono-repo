/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/HttpSender/ICurlWrapper.h>

namespace Common
{
    namespace HttpSender
    {
        class CurlWrapper : public ICurlWrapper
        {
        public:
            CurlWrapper() = default;
            CurlWrapper(const CurlWrapper&) = delete;
            CurlWrapper& operator= (const CurlWrapper&) = delete;
            ~CurlWrapper() override = default;

            CURLcode curlGlobalInit(long flags) override;
            CURL* curlEasyInit() override;

            CURLcode curlEasySetOpt(CURL* handle, CURLoption option, const char* parameter) override;

            curl_slist* curlSlistAppend(curl_slist* list, const char* value) override;

            CURLcode curlEasyPerform(CURL* handle) override;

            void curlSlistFreeAll(curl_slist* list) override;

            void curlEasyCleanup(CURL* handle) override;
            void curlGlobalCleanup() override;

            const char* curlEasyStrError(CURLcode errornum) override;
        };
    } // namespace HttpSender
} // namespace Common
