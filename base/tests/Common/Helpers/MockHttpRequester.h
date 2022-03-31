/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/HttpRequests/IHttpRequester.h>
#include <gmock/gmock.h>

using namespace ::testing;

class MockHTTPRequester : public Common::HttpRequests::IHttpRequester
{
public:
    MOCK_METHOD1(get, Common::HttpRequests::Response(Common::HttpRequests::RequestConfig request));
    MOCK_METHOD1(post, Common::HttpRequests::Response(Common::HttpRequests::RequestConfig request));
    MOCK_METHOD1(del, Common::HttpRequests::Response(Common::HttpRequests::RequestConfig request));
    MOCK_METHOD1(put, Common::HttpRequests::Response(Common::HttpRequests::RequestConfig request));
    MOCK_METHOD1(options, Common::HttpRequests::Response(Common::HttpRequests::RequestConfig request));
};