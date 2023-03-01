// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockedApplicationPathManager : public Common::ApplicationConfiguration::IApplicationPathManager
{
public:
    MOCK_CONST_METHOD1(getPluginSocketAddress, std::string(const std::string&));
    MOCK_CONST_METHOD0(getManagementAgentSocketAddress, std::string(void));
    MOCK_CONST_METHOD0(getWatchdogSocketAddress, std::string(void));
    MOCK_CONST_METHOD0(getActualUserGroupIdConfigPath, std::string(void));
    MOCK_CONST_METHOD0(getRequestedUserGroupIdConfigPath, std::string(void));
    MOCK_CONST_METHOD0(getEventSubscriberSocketAddress, std::string(void));
    MOCK_CONST_METHOD0(getEventSubscriberSocketFile, std::string(void));
    MOCK_CONST_METHOD0(getEventJournalsPath, std::string(void));
    MOCK_CONST_METHOD0(getResponseActionTmpPath, std::string(void));
    MOCK_CONST_METHOD0(getResponseActionRunnerPath, std::string(void));
    MOCK_CONST_METHOD0(sophosInstall, std::string(void));
    MOCK_CONST_METHOD0(getRelativeLogConfFilePath, std::string(void));
    MOCK_CONST_METHOD0(getRelativeLocalLogConfFilePath, std::string(void));
    MOCK_CONST_METHOD0(getLogConfFilePath, std::string(void));
    MOCK_CONST_METHOD0(getLocalLogConfFilePath, std::string(void));
    MOCK_CONST_METHOD0(getTelemetryOutputFilePath, std::string(void));
    MOCK_CONST_METHOD0(getPublisherDataChannelAddress, std::string(void));
    MOCK_CONST_METHOD0(getSubscriberDataChannelAddress, std::string(void));
    MOCK_CONST_METHOD0(getPluginRegistryPath, std::string(void));
    MOCK_CONST_METHOD0(getVersigPath, std::string(void));
    MOCK_CONST_METHOD0(getMcsPolicyFilePath, std::string(void));
    MOCK_CONST_METHOD0(getInternalPolicyFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMcsActionFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMcsStatusFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMcsEventFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMcsConfigFolderPath, std::string(void));
    MOCK_CONST_METHOD0(getMcsCurrentProxyFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMcsCurrentProxyFileName, std::string(void));
    MOCK_CONST_METHOD0(getMcsFlagsFilePath, std::string(void));

    MOCK_CONST_METHOD0(getManagementAgentStatusCacheFilePath, std::string(void));

    MOCK_CONST_METHOD0(getLocalWarehouseStoreDir, std::string(void));

    MOCK_CONST_METHOD0(getLocalWarehouseRepository, std::string(void));
    MOCK_CONST_METHOD0(getLocalDistributionRepository, std::string(void));

    MOCK_CONST_METHOD0(getLocalSdds3Repository, std::string(void));
    MOCK_CONST_METHOD0(getLocalSdds3DistributionRepository, std::string(void));
    MOCK_CONST_METHOD0(getSdds3PackageConfigPath, std::string(void));
    MOCK_CONST_METHOD0(getSdds3OverrideSettingsFile, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderSynLogPath, std::string(void));

    MOCK_CONST_METHOD0(getLocalUninstallSymLinkPath, std::string(void));
    MOCK_CONST_METHOD0(getLocalVersionSymLinkPath, std::string(void));
    MOCK_CONST_METHOD0(getLocalBaseUninstallerPath, std::string(void));

    MOCK_CONST_METHOD0(getUpdateCertificatesPath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderProcessedReportPath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderPath, std::string(void));
    MOCK_CONST_METHOD0(getBaseExecutablesDir, std::string(void));
    MOCK_CONST_METHOD0(getUpdateCacheCertificateFilePath, std::string(void));
    MOCK_CONST_METHOD0(getTempPath, std::string(void));
    MOCK_CONST_METHOD0(getBaseLogDirectory, std::string(void));
    MOCK_CONST_METHOD0(getBaseSophossplLogDirectory, std::string(void));
    MOCK_CONST_METHOD0(getBaseSophossplConfigFileDirectory, std::string(void));

    MOCK_CONST_METHOD0(getPreXdrUpdateReportPath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderReportPath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderConfigFilePath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderReportGeneratedFilePath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderLockFilePath, std::string(void));
    MOCK_CONST_METHOD0(getSulDownloaderLatestProductUpdateMarkerPath, std::string(void));

    MOCK_CONST_METHOD0(getSavedEnvironmentProxyFilePath, std::string(void));

    MOCK_CONST_METHOD0(getTelemetrySchedulerStatusFilePath, std::string(void));
    MOCK_CONST_METHOD0(getTelemetrySupplementaryFilePath, std::string(void));

    MOCK_CONST_METHOD0(getTelemetryExeConfigFilePath, std::string(void));
    MOCK_CONST_METHOD0(getTelemetryExecutableFilePath, std::string(void));
    MOCK_CONST_METHOD0(getSophosAliasFilePath, std::string(void));

    MOCK_CONST_METHOD0(getAlcPolicyFilePath, std::string(void));
    MOCK_CONST_METHOD0(getAlcStatusFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMachineIdFilePath, std::string(void));
    MOCK_CONST_METHOD0(getVersionFilePath, std::string(void));
    MOCK_CONST_METHOD0(getMcsConfigFilePath, std::string(void));

    MOCK_CONST_METHOD0(getInstalledProductPath, std::string (void));
    MOCK_CONST_METHOD0(getPreviousUpdateConfigFileName, std::string (void));
    MOCK_CONST_METHOD0(getSulDownloaderPreviousConfigFilePath, std::string (void));
    MOCK_CONST_METHOD1(getVersionIniFileForComponent, std::string (const std::string& component));
    MOCK_CONST_METHOD0(getFeaturesJsonPath, std::string (void));
    MOCK_CONST_METHOD0(getStateMachineRawDataPath, std::string (void));

    MOCK_CONST_METHOD0(getRemoteDiagnosePath, std::string (void));
    MOCK_CONST_METHOD0(getDiagnoseOutputPath, std::string (void));

    MOCK_CONST_METHOD0(getCloudMetadataJsonPath, std::string (void));
    MOCK_CONST_METHOD0(getThreatHealthJsonFilePath, std::string (void));
    MOCK_CONST_METHOD0(getShsStatusFilePath, std::string (void));
    MOCK_CONST_METHOD0(getOverallHealthFilePath, std::string (void));
    MOCK_CONST_METHOD0(getMcsCaOverrideFlag, std::string (void));

    MOCK_METHOD(std::string, getOutbreakModeStatusFilePath, (), (const, override));

};
