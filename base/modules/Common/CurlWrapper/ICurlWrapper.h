/******************************************************************************************************

Copyright 2019-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <curl.h>
#include <string>
#include <variant>

namespace Common::CurlWrapper
{
    class ICurlWrapper
    {
    public:
        virtual ~ICurlWrapper() = default;

        virtual CURLcode curlGlobalInit(long flags) = 0;
        virtual CURL* curlEasyInit() = 0;
        virtual void curlEasyReset(CURL* handle) = 0;

        virtual CURLcode curlEasySetOptHeaders(CURL* handle, curl_slist* headers) = 0;

        virtual CURLcode curlEasySetOpt(
            CURL* handle,
            CURLoption option,
            const std::variant<std::string, long> parameter) = 0;

        virtual CURLcode curlEasySetFuncOpt(CURL* handle, CURLoption option, void* funcParam) = 0;
        virtual CURLcode curlEasySetDataOpt(CURL* handle, CURLoption option, void* dataParam) = 0;

        virtual CURLcode curlGetResponseCode(CURL* handle, long* codep) = 0;

        virtual struct curl_slist* curlSlistAppend(curl_slist* list, const std::string& value) = 0;

        virtual CURLcode curlEasyPerform(CURL* handle) = 0;

        virtual void curlSlistFreeAll(curl_slist* list) = 0;

        virtual void curlEasyCleanup(CURL* handle) = 0;
        virtual void curlGlobalCleanup() = 0;

        virtual const char* curlEasyStrError(CURLcode errornum) = 0;
    };
} // namespace Common::CurlWrapper