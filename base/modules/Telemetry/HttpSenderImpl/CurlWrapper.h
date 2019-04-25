/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Telemetry/HttpSender/ICurlWrapper.h>

class CurlWrapper : public ICurlWrapper
{
public:
    CurlWrapper() = default;
    CurlWrapper(const CurlWrapper&) = delete;
    ~CurlWrapper() override = default;

    CURLcode curlGlobalInit(long flags) override;
    CURL* curlEasyInit() override;

    CURLcode curlEasySetopt(CURL* handle, CURLoption option, const char* parameter) override;

    struct curl_slist* curlSlistAppend(struct curl_slist* list, const char* value) override;

    CURLcode curlEasyPerform(CURL* handle) override;

    void curlSlistFreeAll(struct curl_slist* list) override;

    void curlEasyCleanup(CURL* handle) override;
    void curlGlobalCleanup() override;

    const char* curlEasyStrerror(CURLcode errornum) override;
};
