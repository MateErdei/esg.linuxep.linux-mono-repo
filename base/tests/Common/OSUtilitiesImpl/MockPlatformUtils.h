/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/IPlatformUtils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockPlatformUtils : public Common::OSUtilities::IPlatformUtils
{
public:
    MOCK_METHOD0(getHostname, std::string());
    MOCK_METHOD0(getPlatform, std::string());
    MOCK_METHOD0(getVendor, std::string());
    MOCK_METHOD0(getArchitecture, std::string());
    MOCK_METHOD0(getOsName, std::string());
    MOCK_METHOD0(getKernelVersion, std::string());
    MOCK_METHOD0(getOsMajorVersion, std::string());
    MOCK_METHOD0(getOsMinorVersion, std::string());
    MOCK_METHOD0(getDomainname, std::string());
};