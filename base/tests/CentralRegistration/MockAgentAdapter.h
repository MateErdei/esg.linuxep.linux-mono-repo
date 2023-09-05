// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "cmcsrouter/IAdapter.h"

#include <gmock/gmock.h>

using namespace ::testing;

class MockAgentAdapter : public MCS::IAdapter
{
public:
    MOCK_METHOD(std::string, getStatusXml, ((std::map<std::string, std::string>)& configOptions), (const, override));
    MOCK_METHOD(std::string, getStatusXml, ((std::map<std::string, std::string>)& configOptions, const std::shared_ptr<Common::HttpRequests::IHttpRequester>&), (const, override));
};
