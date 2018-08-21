/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


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
    MOCK_CONST_METHOD0(getMcsPolicyFilePath, std::string (void));
    MOCK_CONST_METHOD0(getMcsActionFilePath, std::string (void));
    MOCK_CONST_METHOD0(getMcsStatusFilePath, std::string (void));
    MOCK_CONST_METHOD0(getMcsEventFilePath, std::string (void));

    MOCK_CONST_METHOD0(getManagementAgentStatusCacheFilePath, std::string (void));

    MOCK_CONST_METHOD0(getLocalWarehouseRepository, std::string(void));
    MOCK_CONST_METHOD0(getLocalDistributionRepository, std::string(void));
    MOCK_CONST_METHOD0(getLocalUninstallSymLinkPath, std::string(void));

    MOCK_CONST_METHOD0(getUpdateCertificatesPath, std::string(void));
    MOCK_CONST_METHOD0(getUpdateCacheCertificateFilePath, std::string(void));
    MOCK_CONST_METHOD0(getTempPath, std::string(void));
    MOCK_CONST_METHOD0(getBaseLogDirectory, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderReportPath, std::string (void));

    MOCK_CONST_METHOD0(getSulDownloaderConfigFilePath, std::string(void));

    MOCK_CONST_METHOD0(getSulDownloaderReportGeneratedFilePath, std::string(void));
};

