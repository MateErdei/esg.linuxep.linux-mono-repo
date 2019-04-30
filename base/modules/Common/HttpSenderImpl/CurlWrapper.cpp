/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CurlWrapper.h"
namespace Common
{
    namespace HttpSender
    {
        CURLcode CurlWrapper::curlGlobalInit(long flags)
        {
            return curl_global_init(flags);
        }

        CURL* CurlWrapper::curlEasyInit()
        {
            return curl_easy_init();
        }

        CURLcode CurlWrapper::curlEasySetOpt(CURL* handle, CURLoption option, const char* parameter)
        {
            return curl_easy_setopt(handle, option, parameter);
        }

        curl_slist* CurlWrapper::curlSlistAppend(curl_slist* list, const char* value)
        {
            return curl_slist_append(list, value);
        }

        CURLcode CurlWrapper::curlEasyPerform(CURL* handle)
        {
            return curl_easy_perform(handle);
        }

        void CurlWrapper::curlSlistFreeAll(curl_slist* list)
        {
            curl_slist_free_all(list);
        }

        void CurlWrapper::curlEasyCleanup(CURL* handle)
        {
            curl_easy_cleanup(handle);
        }

        void CurlWrapper::curlGlobalCleanup()
        {
            curl_global_cleanup();
        }

        const char* CurlWrapper::curlEasyStrError(CURLcode errornum)
        {
            return curl_easy_strerror(errornum);
        }
    } // namespace HttpSender
} // namespace Common
