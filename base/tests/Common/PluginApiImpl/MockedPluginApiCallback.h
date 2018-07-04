/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
#define EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
#include "Common/PluginApi/IPluginCallback.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;


class MockedPluginApiCallback: public Common::PluginApi::IPluginCallback
{
public:
    MOCK_METHOD1(applyNewPolicy, void(const std::string &));
    MOCK_METHOD1(doAction, void(const std::string &));
    MOCK_METHOD0(shutdown, void(void));
    MOCK_METHOD2(getStatus, void( std::string&,  std::string&));
    MOCK_METHOD0(getTelemetry, std::string(void));
};
#endif //EVEREST_BASE_MOCKEDPLUGINAPICALLBACK_H
