/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginCommunication/IPluginProxy.h>
#include <gmock/gmock.h>

#include <string>

using namespace ::testing;

class MockPluginProxy : public Common::PluginCommunication::IPluginProxy
{
public:
    MOCK_METHOD2(applyNewPolicy, void(const std::string&, const std::string&));
    MOCK_METHOD3(queueAction, void(const std::string&, const std::string&, const std::string&));
    MOCK_METHOD0(getStatus, std::vector<Common::PluginApi::StatusInfo>());
    MOCK_METHOD0(getTelemetry, std::string());
    MOCK_METHOD0(saveTelemetry, void(void));
    MOCK_METHOD1(setPolicyAndActionsAppIds, void(const std::vector<std::string>&));
    MOCK_METHOD1(setStatusAppIds, void(const std::vector<std::string>&));
    MOCK_METHOD1(hasPolicyAppId, bool(const std::string&));
    MOCK_METHOD1(hasActionAppId, bool(const std::string&));
    MOCK_METHOD1(hasStatusAppId, bool(const std::string&));
    MOCK_METHOD0(name, std::string());
};
