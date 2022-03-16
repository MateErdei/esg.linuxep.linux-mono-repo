/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

#include <cstdlib>
#include <string>
#include <curl.h>
#include "HttpRequesterImpl.h"

class CurlFunctionsProvider
{
public:
    static size_t curlWriteFunc(void* ptr, size_t size, size_t nmemb, std::string* buffer);

    static size_t curlWriteFileFunc(void* ptr, size_t size, size_t nmemb, Common::HttpRequestsImpl::ResponseBuffer* responseBuffer) ;

    static size_t curlWriteHeadersFunc(void* ptr, size_t size, size_t nmemb, Common::HttpRequestsImpl::ResponseBuffer* responseBuffer);

    static int curlWriteDebugFunc(      CURL* handle,        curl_infotype type,        char* data,        size_t size,       void* userp);

    static size_t curlFileReadFunc(char *ptr, size_t size, size_t nmemb, FILE *stream);
};

