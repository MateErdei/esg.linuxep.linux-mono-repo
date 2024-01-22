// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockedApplicationPathManager : public Common::ApplicationConfiguration::IApplicationPathManager
{
public:
    MOCK_METHOD(std::string, getPluginSocketAddress, (const std::string& pluginName), (const, override));
    MOCK_METHOD(std::string, getManagementAgentSocketAddress, (), (const, override));
    MOCK_METHOD(std::string, getWatchdogSocketAddress, (), (const, override));
    MOCK_METHOD(std::string, getActualUserGroupIdConfigPath, (), (const, override));
    MOCK_METHOD(std::string, getRequestedUserGroupIdConfigPath, (), (const, override));
    MOCK_METHOD(std::string, getEventSubscriberSocketAddress, (), (const, override));
    MOCK_METHOD(std::string, getEventSubscriberSocketFile, (), (const, override));
    MOCK_METHOD(std::string, getEventJournalsPath, (), (const, override));
    MOCK_METHOD(std::string, getResponseActionTmpPath, (), (const, override));
    MOCK_METHOD(std::string, getResponseActionRunnerPath, (), (const, override));
    MOCK_METHOD(std::string, sophosInstall, (), (const, override));
    MOCK_METHOD(std::string, getRelativeLogConfFilePath, (), (const, override));
    MOCK_METHOD(std::string, getRelativeLocalLogConfFilePath, (), (const, override));
    MOCK_METHOD(std::string, getLogConfFilePath, (), (const, override));
    MOCK_METHOD(std::string, getLocalLogConfFilePath, (), (const, override));
    MOCK_METHOD(std::string, getTelemetryRestoreDir, (), (const, override));
    MOCK_METHOD(std::string, getTelemetryOutputFilePath, (), (const, override));
    MOCK_METHOD(std::string, getPublisherDataChannelAddress, (), (const));
    MOCK_METHOD(std::string, getSubscriberDataChannelAddress, (), (const));
    MOCK_METHOD(std::string, getPluginRegistryPath, (), (const, override));
    MOCK_METHOD(std::string, getVersigPath, (), (const, override));
    MOCK_METHOD(std::string, getMcsPolicyFilePath, (), (const, override));
    MOCK_METHOD(std::string, getInternalPolicyFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsActionFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsStatusFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsEventFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsConfigFolderPath, (), (const, override));
    MOCK_METHOD(std::string, getMcsCurrentProxyFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsCurrentProxyFileName, (), (const, override));
    MOCK_METHOD(std::string, getMcsFlagsFilePath, (), (const, override));

    MOCK_METHOD(std::string, getManagementAgentStatusCacheFilePath, (), (const, override));

    MOCK_METHOD(std::string, getLocalWarehouseStoreDir, (), (const, override));

    MOCK_METHOD(std::string, getLocalWarehouseRepository, (), (const, override));
    MOCK_METHOD(std::string, getLocalDistributionRepository, (), (const, override));

    MOCK_METHOD(std::string, getLocalSdds3Repository, (), (const, override));
    MOCK_METHOD(std::string, getLocalSdds3DistributionRepository, (), (const, override));
    MOCK_METHOD(std::string, getSdds3PackageConfigPath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderInstalledTrackerFile, (), (const, override));
    MOCK_METHOD(std::string, getSdds3OverrideSettingsFile, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderSynLogPath, (), (const, override));

    MOCK_METHOD(std::string, getLocalUninstallSymLinkPath, (), (const, override));
    MOCK_METHOD(std::string, getLocalVersionSymLinkPath, (), (const, override));
    MOCK_METHOD(std::string, getLocalBaseUninstallerPath, (), (const, override));
    MOCK_METHOD(std::string, getWdctlPath, (), (const, override));

    MOCK_METHOD(std::string, getUpdateCertificatesPath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderProcessedReportPath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderPath, (), (const, override));
    MOCK_METHOD(std::string, getBaseExecutablesDir, (), (const, override));
    MOCK_METHOD(std::string, getUpdateCacheCertificateFilePath, (), (const, override));
    MOCK_METHOD(std::string, getTempPath, (), (const, override));
    MOCK_METHOD(std::string, getBaseLogDirectory, (), (const, override));
    MOCK_METHOD(std::string, getBaseSophossplLogDirectory, (), (const, override));
    MOCK_METHOD(std::string, getBaseSophossplConfigFileDirectory, (), (const, override));

    MOCK_METHOD(std::string, getPreXdrUpdateReportPath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderReportPath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderConfigFilePath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderReportGeneratedFilePath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderLockFilePath, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderLatestProductUpdateMarkerPath, (), (const, override));

    MOCK_METHOD(std::string, getSavedEnvironmentProxyFilePath, (), (const, override));

    MOCK_METHOD(std::string, getTelemetrySchedulerStatusFilePath, (), (const, override));
    MOCK_METHOD(std::string, getTelemetryStatusFilePath, (), (const, override));
    MOCK_METHOD(std::string, getTelemetrySupplementaryFilePath, (), (const, override));

    MOCK_METHOD(std::string, getTelemetryExeConfigFilePath, (), (const, override));
    MOCK_METHOD(std::string, getTelemetryExecutableFilePath, (), (const, override));

    MOCK_METHOD(std::string, getAlcPolicyFilePath, (), (const, override));
    MOCK_METHOD(std::string, getAlcStatusFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMachineIdFilePath, (), (const, override));
    MOCK_METHOD(std::string, getVersionFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsConfigFilePath, (), (const, override));

    MOCK_METHOD(std::string, getInstalledProductPath, (), (const));
    MOCK_METHOD(std::string, getPreviousUpdateConfigFileName, (), (const, override));
    MOCK_METHOD(std::string, getSulDownloaderPreviousConfigFilePath, (), (const, override));
    MOCK_METHOD(std::string, getVersionIniFileForComponent, (const std::string& component), (const, override));
    MOCK_METHOD(std::string, getFeaturesJsonPath, (), (const, override));
    MOCK_METHOD(std::string, getLastUpdateStartTimeMarkerPath, (), (const, override));
    MOCK_METHOD(std::string, getForcedAnUpdateMarkerPath, (), (const, override));
    MOCK_METHOD(std::string, getForcedAPausedUpdateMarkerPath, (), (const, override));
    MOCK_METHOD(std::string, getStateMachineRawDataPath, (), (const, override));

    MOCK_METHOD(std::string, getRemoteDiagnosePath, (), (const, override));
    MOCK_METHOD(std::string, getDiagnoseOutputPath, (), (const, override));

    MOCK_METHOD(std::string, getCloudMetadataJsonPath, (), (const, override));

    MOCK_METHOD(std::string, getUpdateMarkerFile, (), (const, override));
    MOCK_METHOD(std::string, getThreatHealthJsonFilePath, (), (const, override));
    MOCK_METHOD(std::string, getShsStatusFilePath, (), (const, override));
    MOCK_METHOD(std::string, getOverallHealthFilePath, (), (const, override));
    MOCK_METHOD(std::string, getMcsCaOverrideFlag, (), (const, override));

    MOCK_METHOD(std::string, getOutbreakModeStatusFilePath, (), (const, override));
    MOCK_METHOD(std::string, getProductInstallLogFilePath, (const std::string& productName), (const, override));
    MOCK_METHOD(std::string, getProductInstallFailedLogFilePath, (const std::string& productName), (const, override));

};
