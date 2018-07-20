/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_MOCKEDAPPLICATIONPATHMANAGER_H
#define EVEREST_BASE_MOCKEDAPPLICATIONPATHMANAGER_H
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace ::testing;


class MockedApplicationPathManager: public Common::ApplicationConfiguration::IApplicationPathManager
{
public:
    MOCK_CONST_METHOD1(getPluginSocketAddress, std::string(const std::string &));
    MOCK_CONST_METHOD0(getManagementAgentSocketAddress, std::string(void));
    MOCK_CONST_METHOD0(getWatchdogSocketAddress, std::string(void));
    MOCK_CONST_METHOD0(sophosInstall, std::string(void));
    MOCK_CONST_METHOD0(getPublisherDataChannelAddress, std::string(void));
    MOCK_CONST_METHOD0(getSubscriberDataChannelAddress, std::string(void));
    MOCK_CONST_METHOD0(getPluginRegistryPath, std::string (void));
    MOCK_CONST_METHOD0(getVersigPath, std::string (void));
};
#endif //EVEREST_BASE_MOCKEDAPPLICATIONPATHMANAGER_H
