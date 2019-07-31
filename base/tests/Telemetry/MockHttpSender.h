/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/HttpSender/IHttpSender.h>
#include <gmock/gmock.h>

#include <string>

using namespace ::testing;

class MockHttpSender : public Common::HttpSender::IHttpSender
{
public:
    MOCK_METHOD1(doHttpsRequest, int(Common::HttpSenderImpl::RequestConfig& requestConfig));
};