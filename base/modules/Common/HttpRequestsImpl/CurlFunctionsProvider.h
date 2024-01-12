// Copyright 2022-2024 Sophos Limited. All rights reserved.

#pragma once

#include "HttpRequesterImpl.h"

#include <cstdlib>
#include <curl.h>
#include <string>

class CurlFunctionsProvider
{
public:
    class WriteBackBuffer
    {
    public:
        std::string buffer_;
        size_t maxSize_ = 0;
        bool tooBig_ = false;
    };
    static size_t curlWriteFunc(void* ptr, size_t size, size_t nmemb, void* buffer);

    static size_t curlWriteFileFunc(
        void* ptr,
        size_t size,
        size_t nmemb,
        Common::HttpRequestsImpl::ResponseBuffer* responseBuffer);

    static size_t curlWriteHeadersFunc(
        void* ptr,
        size_t size,
        size_t nmemb,
        Common::HttpRequestsImpl::ResponseBuffer* responseBuffer);

    static int curlWriteDebugFunc(CURL* handle, curl_infotype type, char* data, size_t size, void* userp);

    static size_t curlFileReadFunc(char* ptr, size_t size, size_t nmemb, FILE* stream);

    static int curlSeekFileFunc(void* userp, curl_off_t offset, int origin);
    static void sendTerminateRequest();
    private:
        static  std::atomic<bool> terminateRequested;
};
