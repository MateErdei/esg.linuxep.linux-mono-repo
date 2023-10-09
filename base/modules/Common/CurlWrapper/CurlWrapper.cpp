/******************************************************************************************************

Copyright 2019-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CurlWrapper.h"

#include <mutex>

namespace
{
    bool g_curlInitialised = false;
}

namespace Common::CurlWrapper
{
    CURLcode CurlWrapper::curlGlobalInit(long flags)
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock{ mutex };
        if (g_curlInitialised)
        {
            return CURLE_OK;
        }
        CURLcode curlCode = curl_global_init(flags);
        if (curlCode == CURLE_OK)
        {
            g_curlInitialised = true;
        }
        return curlCode;
    }

    CURL* CurlWrapper::curlEasyInit() { return curl_easy_init(); }

    CURLcode CurlWrapper::curlEasySetOptHeaders(CURL* handle, curl_slist* headers)
    {
        return curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode CurlWrapper::curlEasySetOpt(CURL* handle, CURLoption option, std::variant<std::string, long> parameter)
    {
        if (std::holds_alternative<std::string>(parameter))
        {
            return curl_easy_setopt(handle, option, std::get<std::string>(parameter).c_str());
        }
        return curl_easy_setopt(handle, option, std::get<long>(parameter));
    }

    curl_slist* CurlWrapper::curlSlistAppend(curl_slist* list, const std::string& value)
    {
        return curl_slist_append(list, value.c_str());
    }

    CURLcode CurlWrapper::curlGetResponseCode(CURL* handle, long* codep)
    {
        return curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, codep);
    }

    CURLcode CurlWrapper::curlEasyPerform(CURL* handle) { return curl_easy_perform(handle); }

    void CurlWrapper::curlSlistFreeAll(curl_slist* list) { curl_slist_free_all(list); }

    void CurlWrapper::curlEasyCleanup(CURL* handle) { curl_easy_cleanup(handle); }

    void CurlWrapper::curlGlobalCleanup() { curl_global_cleanup(); }

    const char* CurlWrapper::curlEasyStrError(CURLcode errornum) { return curl_easy_strerror(errornum); }

    void CurlWrapper::curlEasyReset(CURL* handle) { curl_easy_reset(handle); }
} // namespace Common::CurlWrapper
