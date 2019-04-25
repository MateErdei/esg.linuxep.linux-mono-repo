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

    MOCK_METHOD2(getRequest, int(const std::vector<std::string>& additionalHeaders,
                                 const std::string& certPath));
    MOCK_METHOD3(postRequest, int(const std::vector<std::string>& additionalHeaders,
                                  const std::string& jsonStruct,
                                  const std::string& certPath));
    MOCK_METHOD3(putRequest, int(const std::vector<std::string>& additionalHeaders,
                                  const std::string& jsonStruct,
                                  const std::string& certPath));
};