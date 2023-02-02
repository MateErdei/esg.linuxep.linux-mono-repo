// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ApplicationPathManager.h"

#include "IApplicationConfiguration.h"

#include "Common/FileSystem/IFileSystem.h"

#include <stdexcept>

namespace Common
{
    namespace ApplicationConfigurationImpl
    {
        std::string ApplicationPathManager::getPluginSocketAddress(const std::string& pluginName) const
        {
            return socketPath("plugins/" + pluginName + ".ipc");
        }

        std::string ApplicationPathManager::getEventSubscriberSocketAddress() const
        {
            return socketPath("events.ipc");
        }
        std::string ApplicationPathManager::getEventSubscriberSocketFile() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/ipc/events.ipc");
        }

        std::string ApplicationPathManager::getEventJournalsPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "plugins/eventjournaler", "data/eventjournals");
        }

        std::string ApplicationPathManager::getManagementAgentSocketAddress() const
        {
            return socketPath("mcs_agent.ipc");
        }

        std::string ApplicationPathManager::getWatchdogSocketAddress() const { return socketPath("watchdog.ipc"); }

        std::string ApplicationPathManager::sophosInstall() const
        {
            return Common::ApplicationConfiguration::applicationConfiguration().getData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL);
        }

        std::string ApplicationPathManager::socketPath(const std::string& relative) const
        {
            try
            {
                // Allow an override with the relative path
                return Common::ApplicationConfiguration::applicationConfiguration().getData(relative);
            }
            catch (const std::out_of_range&)
            {
                // No override, so return the constructed value.
                // If the exception is a performance problem, then provide a getData that doesn't throw
                return "ipc://" + sophosInstall() + "/var/ipc/" + relative;
            }
        }


        std::string ApplicationPathManager::getPluginRegistryPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/pluginRegistry/");
        }

        std::string ApplicationPathManager::getVersigPath() const
        {
            std::string versigPath = Common::FileSystem::join(sophosInstall(), "base/update/versig");
            if (Common::FileSystem::fileSystem()->isFile(versigPath))
            {
                return versigPath;
            }
            auto envVersigPATH = ::secure_getenv("VERSIGPATH");
            if (envVersigPATH == nullptr)
            {
                return versigPath;
            }
            return envVersigPATH;
        }

        std::string ApplicationPathManager::getMcsPolicyFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/policy");
        }
        std::string ApplicationPathManager::getInternalPolicyFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/internal_policy");
        }
        std::string ApplicationPathManager::getMcsActionFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/action");
        }

        std::string ApplicationPathManager::getMcsStatusFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/status");
        }

        std::string ApplicationPathManager::getMcsEventFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/event");
        }

        std::string ApplicationPathManager::getMcsConfigFolderPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/sophosspl");
        }
        std::string ApplicationPathManager::getMcsCurrentProxyFilePath() const
        {
            return Common::FileSystem::join(getMcsConfigFolderPath(), getMcsCurrentProxyFileName());
        }
        std::string ApplicationPathManager::getMcsFlagsFilePath() const
        {
            return Common::FileSystem::join(getMcsConfigFolderPath(), "flags-mcs.json");
        }
        std::string ApplicationPathManager::getManagementAgentStatusCacheFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/status/cache");
        }

        std::string ApplicationPathManager::getTempPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "tmp");
        }

        std::string ApplicationPathManager::getLocalWarehouseStoreDir() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/");
        }

        std::string ApplicationPathManager::getLocalWarehouseRepository() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/primarywarehouse");
        }

        std::string ApplicationPathManager::getLocalDistributionRepository() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/primary");
        }

        std::string ApplicationPathManager::getLocalSdds3Repository() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/sdds3primaryrepository");
        }
        std::string ApplicationPathManager::getLocalSdds3DistributionRepository() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/sdds3primary");
        }

        std::string ApplicationPathManager::getSdds3PackageConfigPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/package_config.xml");
        }
        std::string ApplicationPathManager::getSdds3OverrideSettingsFile() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/sdds3_override_settings.ini");
        }

        std::string ApplicationPathManager::getSulDownloaderSynLogPath() const
        {
            return Common::FileSystem::join(getBaseLogDirectory(), "suldownloader_sync.log");
        }

        std::string ApplicationPathManager::getLocalUninstallSymLinkPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/installedproducts");
        }

        std::string ApplicationPathManager::getLocalVersionSymLinkPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/installedproductversions");
        }

        std::string ApplicationPathManager::getLocalBaseUninstallerPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "bin/uninstall.sh");
        }

        std::string ApplicationPathManager::getUpdateCertificatesPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/rootcerts");
        }

        std::string ApplicationPathManager::getBaseLogDirectory() const
        {
            return Common::FileSystem::join(sophosInstall(), "logs", "base");
        }

        std::string ApplicationPathManager::getBaseSophossplLogDirectory() const
        {
            return Common::FileSystem::join(getBaseLogDirectory(), "sophosspl");
        }

        std::string ApplicationPathManager::getBaseSophossplConfigFileDirectory() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/sophosspl");
        }

        std::string ApplicationPathManager::getUpdateCacheCertificateFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/updatecachecerts", "cache_certificates.crt");
        }

        std::string ApplicationPathManager::getPreXdrUpdateReportPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/update_report.json");
        }

        std::string ApplicationPathManager::getSulDownloaderReportPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/updatescheduler");
        }

        std::string ApplicationPathManager::getSulDownloaderPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/bin/SulDownloader");
        }

        std::string ApplicationPathManager::getBaseExecutablesDir() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/bin/");
        }

        std::string ApplicationPathManager::getSulDownloaderProcessedReportPath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "processedReports");
        }

        std::string ApplicationPathManager::getSulDownloaderConfigFilePath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "update_config.json");
        }

        std::string ApplicationPathManager::getStateMachineRawDataPath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "state_machine_raw_data.json");
        }

        std::string ApplicationPathManager::getSulDownloaderReportGeneratedFilePath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "update_report.json");
        }

        std::string ApplicationPathManager::getSulDownloaderLockFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/lock/suldownloader.pid");
        }

        std::string ApplicationPathManager::getSulDownloaderLatestProductUpdateMarkerPath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "last_product_update.marker");
        }

        std::string ApplicationPathManager::getSavedEnvironmentProxyFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/savedproxy.config");
        }

        std::string ApplicationPathManager::getLogConfFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), getRelativeLogConfFilePath());
        }

        std::string ApplicationPathManager::getLocalLogConfFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), getRelativeLocalLogConfFilePath());
        }

        std::string ApplicationPathManager::getRelativeLogConfFilePath() const
        {
            return "base/etc/logger.conf";
        }

        std::string ApplicationPathManager::getRelativeLocalLogConfFilePath() const
        {
            return "base/etc/logger.conf.local";
        }

        std::string ApplicationPathManager::getTelemetryOutputFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/telemetry/var/telemetry.json");
        }

        std::string ApplicationPathManager::getTelemetrySchedulerConfigFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/telemetry/var/tscheduler-status.json");
        }

        std::string ApplicationPathManager::getTelemetrySupplementaryFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/telemetry-config.json");
        }

        std::string ApplicationPathManager::getTelemetryExeConfigFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/telemetry/var/telemetry-exe.json");
        }

        std::string ApplicationPathManager::getTelemetryExecutableFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/bin/telemetry");
        }

        std::string ApplicationPathManager::getSophosAliasFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/sophos_alias.txt");
        }

        std::string ApplicationPathManager::getAlcStatusFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/status/ALC_status.xml");
        }

        std::string ApplicationPathManager::getAlcPolicyFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/policy/ALC-1_policy.xml");
        }

        std::string ApplicationPathManager::getMachineIdFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/machine_id.txt");
        }

        std::string ApplicationPathManager::getVersionFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/VERSION.ini");
        }

        std::string ApplicationPathManager::getMcsConfigFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/sophosspl/mcs.config");
        }

        std::string ApplicationPathManager::getPreviousUpdateConfigFileName() const
        {
            return "previous_update_config.json";
        }

        std::string ApplicationPathManager::getSulDownloaderPreviousConfigFilePath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), getPreviousUpdateConfigFileName());
        }

        std::string ApplicationPathManager::getCommsRequestDirPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/comms/requests");
        }

        std::string ApplicationPathManager::getCommsResponseDirPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/comms/responses");
        }

        std::string ApplicationPathManager::getVersionIniFileForComponent(const std::string& component) const
        {
            std::string path;
            if (component == "ServerProtectionLinux-Base" or component == "ServerProtectionLinux-Base-component")
            {
                path = Common::FileSystem::join(sophosInstall(), "base/VERSION.ini");
            }
            else
            {
                path = Common::FileSystem::join(getLocalVersionSymLinkPath(), component + ".ini");
            }

            return path;
        }

        std::string ApplicationPathManager::getFeaturesJsonPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/updatescheduler/installed_features.json");
        }

        std::string ApplicationPathManager::getRemoteDiagnosePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/remote-diagnose");
        }

        std::string ApplicationPathManager::getDiagnoseOutputPath() const
        {
            return Common::FileSystem::join(getRemoteDiagnosePath(), "output");
        }

        std::string ApplicationPathManager::getCloudMetadataJsonPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/etc/sophosspl/instance-metadata.json");
        }

        std::string ApplicationPathManager::getUpdateMarkerFile() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/sophosspl/upgrade_marker_file");
        }

        std::string ApplicationPathManager::getThreatHealthJsonFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/sophosspl/ThreatHealth.json");
        }

        std::string ApplicationPathManager::getShsStatusFilePath() const
        {
            return Common::FileSystem::join(getMcsStatusFilePath(), "SHS_status.xml");
        }

        std::string ApplicationPathManager::getOverallHealthFilePath() const
        {
            return Common::FileSystem::join(getInternalPolicyFilePath(), "internal_EPHEALTH.json");
        }
        std::string ApplicationPathManager::getMcsCurrentProxyFileName() const
        {
            return "current_proxy";
        }
        std::string ApplicationPathManager::getMcsCaOverrideFlag() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/certs/ca_env_override_flag");
        }
        std::string ApplicationPathManager::getOutbreakModeStatusFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/sophosspl/outbreak_status.json");
        }

    } // namespace ApplicationConfigurationImpl

    namespace ApplicationConfiguration
    {
        std::unique_ptr<IApplicationPathManager>& instance()
        {
            static std::unique_ptr<IApplicationPathManager> pointer =
                std::unique_ptr<IApplicationPathManager>(new ApplicationConfigurationImpl::ApplicationPathManager());
            return pointer;
        }

        IApplicationPathManager& applicationPathManager() { return *instance(); }
        /** Use only for test */
        void replaceApplicationPathManager(std::unique_ptr<IApplicationPathManager> applicationPathManager)
        {
            instance().reset(applicationPathManager.release()); // NOLINT
        }
        void restoreApplicationPathManager()
        {
            instance() = std::make_unique<ApplicationConfigurationImpl::ApplicationPathManager>();
        }

    } // namespace ApplicationConfiguration

} // namespace Common