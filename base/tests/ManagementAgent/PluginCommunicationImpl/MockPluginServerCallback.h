/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <ManagementAgent/PluginCommunication/IPluginServerCallback.h>

using namespace ::testing;

class MockPluginServerCallback : virtual public ManagementAgent::PluginCommunication::IPluginServerCallback
{
public:
    MOCK_METHOD2(receivedSendEvent, void(const std::string&, const std::string&));
    MOCK_METHOD2(receivedChangeStatus, void(const std::string&, const Common::PluginApi::StatusInfo&));
    MOCK_METHOD0(shutdown, void());

    MOCK_METHOD1(receivedGetPolicyRequest, bool(const std::string& appId));
    MOCK_METHOD1(receivedRegisterWithManagementAgent, void(const std::string& pluginName));
    MOCK_METHOD3(receivedThreatHealth, void(const std::string& pluginName, const std::string& threatHealth, std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj));
};
