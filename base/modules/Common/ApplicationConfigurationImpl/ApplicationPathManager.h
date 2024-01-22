// Copyright 2018-2024 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

namespace Common::ApplicationConfigurationImpl
{
    class ApplicationPathManager : public virtual Common::ApplicationConfiguration::IApplicationPathManager
    {
    public:
        std::string getPluginSocketAddress(const std::string& pluginName) const override;
        std::string getManagementAgentSocketAddress() const override;
        std::string getWatchdogSocketAddress() const override;
        std::string getActualUserGroupIdConfigPath() const override;
        std::string getRequestedUserGroupIdConfigPath() const override;
        std::string getEventSubscriberSocketAddress() const override;
        std::string getEventSubscriberSocketFile() const override;
        std::string getEventJournalsPath() const override;
        std::string getResponseActionTmpPath() const override;
        std::string getResponseActionRunnerPath() const override;
        std::string sophosInstall() const override;
        std::string getRelativeLogConfFilePath() const override;
        std::string getRelativeLocalLogConfFilePath() const override;
        std::string getLogConfFilePath() const override;
        std::string getLocalLogConfFilePath() const override;
        std::string getTelemetryOutputFilePath() const override;
        std::string getPluginRegistryPath() const override;
        std::string getVersigPath() const override;
        std::string getMcsPolicyFilePath() const override;
        std::string getInternalPolicyFilePath() const override;
        std::string getMcsActionFilePath() const override;
        std::string getMcsStatusFilePath() const override;
        std::string getMcsEventFilePath() const override;
        std::string getMcsConfigFolderPath() const override;
        std::string getMcsCurrentProxyFilePath() const override;
        std::string getMcsCurrentProxyFileName() const override;
        std::string getMcsFlagsFilePath() const override;
        std::string getManagementAgentStatusCacheFilePath() const override;

        std::string getLocalWarehouseStoreDir() const override;
        std::string getLocalWarehouseRepository() const override;
        std::string getLocalDistributionRepository() const override;
        std::string getLocalSdds3Repository() const override;
        std::string getLocalSdds3DistributionRepository() const override;
        std::string getSdds3PackageConfigPath() const override;
        std::string getSulDownloaderInstalledTrackerFile() const override;
        std::string getSdds3OverrideSettingsFile() const override;
        std::string getSulDownloaderSynLogPath() const override;
        std::string getLocalUninstallSymLinkPath() const override;
        std::string getLocalVersionSymLinkPath() const override;
        std::string getLocalBaseUninstallerPath() const override;
        std::string getWdctlPath() const override;

        std::string getUpdateCertificatesPath() const override;
        std::string getUpdateCacheCertificateFilePath() const override;

        std::string getBaseLogDirectory() const override;
        std::string getBaseSophossplLogDirectory() const override;

        std::string getBaseSophossplConfigFileDirectory() const override;

        std::string getTempPath() const override;

        std::string getPreXdrUpdateReportPath() const override;
        std::string getSulDownloaderReportPath() const override;
        std::string getSulDownloaderPath() const override;
        std::string getBaseExecutablesDir() const override;
        std::string getSulDownloaderProcessedReportPath() const override;
        std::string getSulDownloaderConfigFilePath() const override;
        std::string getSulDownloaderReportGeneratedFilePath() const override;
        std::string getSulDownloaderLockFilePath() const override;
        [[nodiscard]] std::string getSulDownloaderLatestProductUpdateMarkerPath() const override;
        std::string getStateMachineRawDataPath() const override;

        std::string getSavedEnvironmentProxyFilePath() const override;

        std::string getTelemetryRestoreDir() const override;
        std::string getTelemetrySchedulerStatusFilePath() const override;
        std::string getTelemetryStatusFilePath() const override;
        std::string getTelemetrySupplementaryFilePath() const override;
        std::string getTelemetryExeConfigFilePath() const override;
        std::string getTelemetryExecutableFilePath() const override;

        std::string getAlcStatusFilePath() const override;
        std::string getAlcPolicyFilePath() const override;
        std::string getMachineIdFilePath() const override;
        std::string getVersionFilePath() const override;
        std::string getMcsConfigFilePath() const override;
        std::string getPreviousUpdateConfigFileName() const override;
        std::string getSulDownloaderPreviousConfigFilePath() const override;

        std::string getVersionIniFileForComponent(const std::string& component) const override;
        std::string getFeaturesJsonPath() const override;
        std::string getLastUpdateStartTimeMarkerPath() const override;
        std::string getForcedAnUpdateMarkerPath() const override;
        std::string getForcedAPausedUpdateMarkerPath() const override;

        std::string getRemoteDiagnosePath() const override;
        std::string getDiagnoseOutputPath() const override;

        std::string getCloudMetadataJsonPath() const override;
        std::string getUpdateMarkerFile() const override;
        std::string getThreatHealthJsonFilePath() const override;
        std::string getShsStatusFilePath() const override;
        std::string getOverallHealthFilePath() const override;
        std::string getMcsCaOverrideFlag() const override;

        [[nodiscard]] std::string getOutbreakModeStatusFilePath() const override;
        [[nodiscard]] std::string getProductInstallLogFilePath(const std::string& productName) const override;
        [[nodiscard]] std::string getProductInstallFailedLogFilePath(const std::string& productName) const override;

    private:
        std::string socketPath(const std::string& relative) const;
    };
} // namespace Common::ApplicationConfigurationImpl
