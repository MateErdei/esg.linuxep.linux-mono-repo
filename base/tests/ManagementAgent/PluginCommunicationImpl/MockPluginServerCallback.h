//
// Created by pair on 06/07/18.
//

#ifndef EVEREST_BASE_MOCKPLUGINSERVERCALLBACK_H
#define EVEREST_BASE_MOCKPLUGINSERVERCALLBACK_H

#include <ManagementAgent/PluginCommunication/IPluginServerCallback.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;

class MockPluginServerCallback : virtual public ManagementAgent::PluginCommunication::IPluginServerCallback
{
public:
    MOCK_METHOD2(receivedSendEvent, void (const std::string&, const std::string &));
    MOCK_METHOD2(receivedChangeStatus, void (const std::string&, const Common::PluginApi::StatusInfo &));
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD1(receivedGetPolicy, std::string (const std::string &));
    MOCK_METHOD1(receivedRegisterWithManagementAgent, void (const std::string &pluginName));
};

#endif //EVEREST_BASE_MOCKPLUGINSERVERCALLBACK_H
