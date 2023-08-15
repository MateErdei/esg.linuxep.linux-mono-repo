// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/OSUtilities/IPlatformUtils.h"
#include <gmock/gmock.h>

using namespace ::testing;

class MockPlatformUtils : public Common::OSUtilities::IPlatformUtils
{
public:
    MOCK_CONST_METHOD0(getHostname, std::string());
    MOCK_METHOD(std::string, getFQDN, (),  (const, override));
    MOCK_CONST_METHOD0(getPlatform, std::string());
    MOCK_CONST_METHOD0(getVendor, std::string());
    MOCK_CONST_METHOD0(getArchitecture, std::string());
    MOCK_CONST_METHOD0(getOsName, std::string());
    MOCK_CONST_METHOD0(getKernelVersion, std::string());
    MOCK_CONST_METHOD0(getOsMajorVersion, std::string());
    MOCK_CONST_METHOD0(getOsMinorVersion, std::string());
    MOCK_CONST_METHOD0(getDomainname, std::string());
    MOCK_CONST_METHOD0(getIp4Address, std::string());
    MOCK_CONST_METHOD0(getIp4Addresses, std::vector<std::string>());
    MOCK_CONST_METHOD0(getIp6Address, std::string());
    MOCK_CONST_METHOD0(getIp6Addresses, std::vector<std::string>());
    MOCK_CONST_METHOD1(getCloudPlatformMetadata, std::string(std::shared_ptr<Common::HttpRequests::IHttpRequester>));
    MOCK_CONST_METHOD0(getMacAddresses, std::vector<std::string>());

};