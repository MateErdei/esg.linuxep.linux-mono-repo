/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
#define EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
#include "Common/PluginApi/IPluginCallbackApi.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;

#ifndef __clang_analyzer__ // avoid clang-tidy errors

class MockedPluginApiCallback: public Common::PluginApi::IPluginCallbackApi
{
public:
    MOCK_METHOD1(applyNewPolicy, void( const std::string &));
    MOCK_METHOD1(queueAction, void( const std::string &));
    MOCK_METHOD0(onShutdown, void(void));

    MOCK_METHOD1(getStatus, Common::PluginApi::StatusInfo(const std::string&));
    MOCK_METHOD0(getTelemetry, std::string(void));
};
#endif
#endif //EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
