/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <cmcsrouter/IAdapter.h>
#include <gmock/gmock.h>

using namespace ::testing;

class MockAgentAdapter : public MCS::IAdapter
{
public:
    MOCK_CONST_METHOD1(getStatusXml, std::string(std::map<std::string, std::string>& configOptions));
};
