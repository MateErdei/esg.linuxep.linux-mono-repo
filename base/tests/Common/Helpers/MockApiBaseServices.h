/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/PluginApi/IBaseServiceApi.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockApiBaseServices : public virtual Common::PluginApi::IBaseServiceApi
{
public:
    MOCK_CONST_METHOD2(sendEvent, void(const std::string&, const std::string&));

    MOCK_CONST_METHOD3(sendStatus, void(const std::string&, const std::string&, const std::string&));

    MOCK_CONST_METHOD1(requestPolicies, void(const std::string&));
};
