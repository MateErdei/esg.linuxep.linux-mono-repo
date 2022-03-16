/******************************************************************************************************

Copyright 2019-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ICurlWrapper.h"

namespace Common::CurlWrapper
{
    class SListScopeGuard
    {
        Common::CurlWrapper::ICurlWrapper& m_iCurlWrapper;
        curl_slist* m_curl_slist;

    public:
        SListScopeGuard(curl_slist* curl_slist, Common::CurlWrapper::ICurlWrapper& iCurlWrapper) :
            m_iCurlWrapper(iCurlWrapper), m_curl_slist(curl_slist)
        {
        }

        ~SListScopeGuard()
        {
            if (m_curl_slist != nullptr)
            {
                m_iCurlWrapper.curlSlistFreeAll(m_curl_slist);
            }
        }
    };

    class CurlWrapper : public ICurlWrapper
    {
    public:
        CurlWrapper() = default;
        CurlWrapper(const CurlWrapper&) = delete;
        CurlWrapper& operator=(const CurlWrapper&) = delete;
        ~CurlWrapper() override = default;

        CURLcode curlGlobalInit(long flags) override;
        CURL* curlEasyInit() override;
        void curlEasyReset(CURL* handle) override;

        CURLcode curlEasySetOptHeaders(CURL* handle, curl_slist* headers) override;
        CURLcode curlEasySetOpt(CURL* handle, CURLoption option, const std::variant<std::string, long> parameter)
            override;
        CURLcode curlGetResponseCode(CURL* handle, long* codep) override;

        curl_slist* curlSlistAppend(curl_slist* list, const std::string& value) override;

        CURLcode curlEasyPerform(CURL* handle) override;

        void curlSlistFreeAll(curl_slist* list) override;

        void curlEasyCleanup(CURL* handle) override;
        void curlGlobalCleanup() override;

        const char* curlEasyStrError(CURLcode errornum) override;
    };
} // namespace Common::CurlWrapper
