/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
#define EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
#include "Common/PluginApi/IPluginCallbackApi.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;


class MockedPluginApiCallback: public Common::PluginApi::IPluginCallbackApi
{
public:
    MOCK_METHOD1(applyNewPolicy, void(const std::string &));
    MOCK_METHOD1(queueAction, void(const std::string &));
    MOCK_METHOD0(onShutdown, void(void));
    MOCK_METHOD0(getStatus, Common::PluginApi::StatusInfo());
    MOCK_METHOD0(getTelemetry, std::string(void));
};
#endif //EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
