// Copyright 2018-2024 Sophos Limited. All rights reserved.

#pragma once

#include <functional>
#include <memory>
#include <string>

namespace Common::ApplicationConfiguration
{
    class IApplicationPathManager
    {
    public:
        virtual ~IApplicationPathManager() = default;

        [[nodiscard]] virtual std::string getPluginSocketAddress(const std::string& pluginName) const = 0;
        [[nodiscard]] virtual std::string getManagementAgentSocketAddress() const = 0;
        [[nodiscard]] virtual std::string getWatchdogSocketAddress() const = 0;
        [[nodiscard]] virtual std::string getActualUserGroupIdConfigPath() const = 0;
        [[nodiscard]] virtual std::string getRequestedUserGroupIdConfigPath() const = 0;
        [[nodiscard]] virtual std::string getEventSubscriberSocketAddress() const = 0;
        [[nodiscard]] virtual std::string getEventSubscriberSocketFile() const = 0;
        [[nodiscard]] virtual std::string getEventJournalsPath() const = 0;
        [[nodiscard]] virtual std::string getResponseActionTmpPath() const = 0;
        [[nodiscard]] virtual std::string getResponseActionRunnerPath() const = 0;
        [[nodiscard]] virtual std::string sophosInstall() const = 0;

        [[nodiscard]] virtual std::string getRelativeLogConfFilePath() const = 0;
        [[nodiscard]] virtual std::string getRelativeLocalLogConfFilePath() const = 0;
        [[nodiscard]] virtual std::string getLogConfFilePath() const = 0;
        [[nodiscard]] virtual std::string getLocalLogConfFilePath() const = 0;
        [[nodiscard]] virtual std::string getTelemetryOutputFilePath() const = 0;
        [[nodiscard]] virtual std::string getVersionIniFileForComponent(const std::string& component) const = 0;

        /**
         * Get the directory to store root logs for base processes.
         * @return
         */
        [[nodiscard]] virtual std::string getBaseLogDirectory() const = 0;
        [[nodiscard]] virtual std::string getBaseSophossplLogDirectory() const = 0;

        [[nodiscard]] virtual std::string getBaseSophossplConfigFileDirectory() const = 0;

        [[nodiscard]] virtual std::string getPluginRegistryPath() const = 0;
        [[nodiscard]] virtual std::string getVersigPath() const = 0;
        [[nodiscard]] virtual std::string getMcsPolicyFilePath() const = 0;
        [[nodiscard]] virtual std::string getInternalPolicyFilePath() const = 0;
        [[nodiscard]] virtual std::string getMcsActionFilePath() const = 0;
        [[nodiscard]] virtual std::string getMcsStatusFilePath() const = 0;
        [[nodiscard]] virtual std::string getMcsEventFilePath() const = 0;
        [[nodiscard]] virtual std::string getMcsConfigFolderPath() const = 0;
        [[nodiscard]] virtual std::string getMcsCurrentProxyFilePath() const = 0;
        [[nodiscard]] virtual std::string getMcsCurrentProxyFileName() const = 0;
        [[nodiscard]] virtual std::string getMcsFlagsFilePath() const = 0;
        [[nodiscard]] virtual std::string getAlcStatusFilePath() const = 0;
        [[nodiscard]] virtual std::string getAlcPolicyFilePath() const = 0;
        [[nodiscard]] virtual std::string getFeaturesJsonPath() const = 0;
        [[nodiscard]] virtual std::string getLastUpdateStartTimeMarkerPath() const = 0;
        [[nodiscard]] virtual std::string getForcedAnUpdateMarkerPath() const = 0;
        [[nodiscard]] virtual std::string getForcedAPausedUpdateMarkerPath() const = 0;

        [[nodiscard]] virtual std::string getManagementAgentStatusCacheFilePath() const = 0;

        [[nodiscard]] virtual std::string getLocalWarehouseStoreDir() const = 0;

        [[nodiscard]] virtual std::string getLocalWarehouseRepository() const = 0;
        [[nodiscard]] virtual std::string getLocalDistributionRepository() const = 0;

        [[nodiscard]] virtual std::string getLocalSdds3Repository() const = 0;
        [[nodiscard]] virtual std::string getLocalSdds3DistributionRepository() const = 0;
        [[nodiscard]] virtual std::string getSdds3PackageConfigPath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderInstalledTrackerFile() const = 0;
        [[nodiscard]] virtual std::string getSdds3OverrideSettingsFile() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderSynLogPath() const = 0;

        /**
         * @brief gets the directory containing the symlinks to the plugin/product uninstallers <ProductLine>.sh for
         * each product <ProductLine> installed.
         * @return the full path of the directory
         */
        [[nodiscard]] virtual std::string getLocalUninstallSymLinkPath() const = 0;
        [[nodiscard]] virtual std::string getLocalVersionSymLinkPath() const = 0;
        [[nodiscard]] virtual std::string getLocalBaseUninstallerPath() const = 0;
        [[nodiscard]] virtual std::string getWdctlPath() const = 0;

        [[nodiscard]] virtual std::string getUpdateCertificatesPath() const = 0;
        [[nodiscard]] virtual std::string getUpdateCacheCertificateFilePath() const = 0;
        [[nodiscard]] virtual std::string getTempPath() const = 0;

        [[nodiscard]] virtual std::string getPreXdrUpdateReportPath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderReportPath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderPath() const = 0;
        [[nodiscard]] virtual std::string getBaseExecutablesDir() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderProcessedReportPath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderConfigFilePath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderReportGeneratedFilePath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderLockFilePath() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderLatestProductUpdateMarkerPath() const = 0;
        [[nodiscard]] virtual std::string getStateMachineRawDataPath() const = 0;
        [[nodiscard]] virtual std::string getSavedEnvironmentProxyFilePath() const = 0;

        [[nodiscard]] virtual std::string getTelemetryRestoreDir() const = 0;
        [[nodiscard]] virtual std::string getTelemetrySchedulerStatusFilePath() const = 0;
        [[nodiscard]] virtual std::string getTelemetryStatusFilePath() const = 0;
        [[nodiscard]] virtual std::string getTelemetrySupplementaryFilePath() const = 0;
        [[nodiscard]] virtual std::string getTelemetryExeConfigFilePath() const = 0;
        [[nodiscard]] virtual std::string getTelemetryExecutableFilePath() const = 0;

        [[nodiscard]] virtual std::string getMachineIdFilePath() const = 0;
        [[nodiscard]] virtual std::string getVersionFilePath() const = 0;
        [[nodiscard]] virtual std::string getMcsConfigFilePath() const = 0;
        [[nodiscard]] virtual std::string getPreviousUpdateConfigFileName() const = 0;
        [[nodiscard]] virtual std::string getSulDownloaderPreviousConfigFilePath() const = 0;

        [[nodiscard]] virtual std::string getRemoteDiagnosePath() const = 0;
        [[nodiscard]] virtual std::string getDiagnoseOutputPath() const = 0;

        [[nodiscard]] virtual std::string getCloudMetadataJsonPath() const = 0;

        [[nodiscard]] virtual std::string getUpdateMarkerFile() const = 0;

        [[nodiscard]] virtual std::string getThreatHealthJsonFilePath() const = 0;

        [[nodiscard]] virtual std::string getShsStatusFilePath() const = 0;

        [[nodiscard]] virtual std::string getOverallHealthFilePath() const = 0;

        [[nodiscard]] virtual std::string getMcsCaOverrideFlag() const = 0;

        [[nodiscard]] virtual std::string getOutbreakModeStatusFilePath() const = 0;

        [[nodiscard]] virtual std::string getProductInstallLogFilePath(const std::string& productName) const = 0;
        [[nodiscard]] virtual std::string getProductInstallFailedLogFilePath(const std::string& productName) const = 0;
    };

    IApplicationPathManager& applicationPathManager();
    /** Use only for test */
    void replaceApplicationPathManager(std::unique_ptr<IApplicationPathManager>);
    void restoreApplicationPathManager();
} // namespace Common::ApplicationConfiguration
