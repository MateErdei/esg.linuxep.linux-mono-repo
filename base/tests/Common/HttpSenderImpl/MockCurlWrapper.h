/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/HttpSender/ICurlWrapper.h>
#include <gmock/gmock.h>

using namespace ::testing;

class MockCurlWrapper : public Common::HttpSender::ICurlWrapper
{
public:
    MOCK_METHOD1(curlGlobalInit, CURLcode(long flags));

    MOCK_METHOD0(curlEasyInit, CURL*());

    MOCK_METHOD2(curlEasySetOptHeaders, CURLcode(CURL* handle, curl_slist* headers));

    MOCK_METHOD3(curlEasySetOpt, CURLcode(CURL* handle, CURLoption option, const std::string& parameter));

    MOCK_METHOD2(curlSlistAppend, curl_slist*(curl_slist* list, const std::string& value));

    MOCK_METHOD1(curlEasyPerform, CURLcode(CURL* handle));

    MOCK_METHOD1(curlSlistFreeAll, void(curl_slist* list));

    MOCK_METHOD1(curlEasyCleanup, void(CURL* handle));

    MOCK_METHOD0(curlGlobalCleanup, void());

    MOCK_METHOD1(curlEasyStrError, const char*(CURLcode errornum));
};