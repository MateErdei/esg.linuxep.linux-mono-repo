/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <gmock/gmock.h>

#include <Telemetry/HttpSender/IHttpSender.h>

#include <string>

using namespace ::testing;

class MockHttpSender : public IHttpSender
{
public:
    MOCK_METHOD1(setServer, void(const std::string& server));
    MOCK_METHOD1(setPort, void(const int& port));

    MOCK_METHOD1(getRequest, int(const std::vector<std::string>& additionalHeaders));
    MOCK_METHOD2(postRequest, int(const std::vector<std::string>& additionalHeaders,
                                    const std::string& jsonStruct));
};