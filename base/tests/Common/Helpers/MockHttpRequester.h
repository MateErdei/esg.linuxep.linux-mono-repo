// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/HttpRequests/IHttpRequester.h"
#include <gmock/gmock.h>

using namespace ::testing;

class MockHTTPRequester : public Common::HttpRequests::IHttpRequester
{
public:
    MOCK_METHOD(Common::HttpRequests::Response, get, (Common::HttpRequests::RequestConfig request));
    MOCK_METHOD(Common::HttpRequests::Response, post, (Common::HttpRequests::RequestConfig request));
    MOCK_METHOD(Common::HttpRequests::Response, del, (Common::HttpRequests::RequestConfig request));
    MOCK_METHOD(Common::HttpRequests::Response, put, (Common::HttpRequests::RequestConfig request));
    MOCK_METHOD(void ,sendTerminate, ());
    MOCK_METHOD(Common::HttpRequests::Response, options, (Common::HttpRequests::RequestConfig request));
};