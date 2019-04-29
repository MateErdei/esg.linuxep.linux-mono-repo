/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Telemetry/HttpSender/IHttpSender.h>

#include <gmock/gmock.h>

#include <string>

using namespace ::testing;

class MockHttpSender : public IHttpSender
{
public:
    MOCK_METHOD1(httpsRequest, int(std::shared_ptr<RequestConfig> requestConfig));
};