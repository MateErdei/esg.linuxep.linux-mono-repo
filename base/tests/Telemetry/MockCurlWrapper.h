/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <gmock/gmock.h>

#include <Telemetry/HttpSender/ICurlWrapper.h>

using namespace ::testing;

class MockCurlWrapper : public ICurlWrapper
{
public:
    MOCK_METHOD1(curlGlobalInit, CURLcode(long flags));

    MOCK_METHOD0(curlEasyInit, CURL*());

    MOCK_METHOD3(curlEasySetopt, CURLcode(CURL* handle, CURLoption option, const char* parameter));

    MOCK_METHOD2(curlSlistAppend, curl_slist*(curl_slist* list, const char* value));

    MOCK_METHOD1(curlEasyPerform, CURLcode(CURL* handle));

    MOCK_METHOD1(curlSlistFreeAll, void(curl_slist * list));

    MOCK_METHOD1(curlEasyCleanup, void(CURL* handle));

    MOCK_METHOD0(curlGlobalCleanup, void());

    MOCK_METHOD1(curlEasyStrerror, const char*(CURLcode errornum));
};