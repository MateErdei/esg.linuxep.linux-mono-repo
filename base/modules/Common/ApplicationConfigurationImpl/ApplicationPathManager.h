/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
        std::string sophosInstall() const override;
        std::string getLogConfFilePath() const override;
        std::string getPublisherDataChannelAddress() const override;
        std::string getSubscriberDataChannelAddress() const override;
        std::string getPluginRegistryPath() const override;
        std::string getVersigPath() const override;
        std::string getMcsPolicyFilePath() const override;
        std::string getMcsActionFilePath() const override;
        std::string getMcsStatusFilePath() const override;
        std::string getMcsEventFilePath() const override;

        std::string getManagementAgentStatusCacheFilePath() const override;

        std::string getLocalWarehouseRepository() const override;
        std::string getLocalDistributionRepository() const override;
        std::string getLocalUninstallSymLinkPath() const override;

        std::string getUpdateCertificatesPath() const override;
        std::string getUpdateCacheCertificateFilePath() const override;

        std::string getBaseLogDirectory() const override;
        std::string getBaseSophossplLogDirectory() const override;

        std::string getBaseSophossplConfigFileDirectory() const override;

        std::string getTempPath() const override;

        std::string getSulDownloaderReportPath() const override;
        std::string getSulDownloaderConfigFilePath() const override;
        std::string getSulDownloaderReportGeneratedFilePath() const override;
        std::string getSulDownloaderLockFilePath() const override;

        std::string getSavedEnvironmentProxyFilePath() const override;

        std::string getTelemetrySchedulerConfigFilePath() const override;
        std::string getTelemetrySupplementaryFilePath() const override;
        std::string getTelemetryExeConfigFilePath() const override;
        std::string getTelemetryExecutableFilePath() const override;

    private:
        std::string socketPath(const std::string& relative) const;
    };
} // namespace Common::ApplicationConfigurationImpl
