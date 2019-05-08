/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <functional>
#include <memory>
#include <string>
namespace Common
{
    namespace ApplicationConfiguration
    {
        class IApplicationPathManager
        {
        public:
            virtual ~IApplicationPathManager() = default;
            virtual std::string getPluginSocketAddress(const std::string& pluginName) const = 0;
            virtual std::string getManagementAgentSocketAddress() const = 0;
            virtual std::string getWatchdogSocketAddress() const = 0;
            virtual std::string sophosInstall() const = 0;

            virtual std::string getLogConfFilePath() const = 0;
            virtual std::string getTelemetryOutputFilePath() const = 0;

            /**
             * Get the directory to store root logs for base processes.
             * @return
             */
            virtual std::string getBaseLogDirectory() const = 0;
            virtual std::string getBaseSophossplLogDirectory() const = 0;

            virtual std::string getPublisherDataChannelAddress() const = 0;
            virtual std::string getSubscriberDataChannelAddress() const = 0;
            virtual std::string getPluginRegistryPath() const = 0;
            virtual std::string getVersigPath() const = 0;
            virtual std::string getMcsPolicyFilePath() const = 0;
            virtual std::string getMcsActionFilePath() const = 0;
            virtual std::string getMcsStatusFilePath() const = 0;
            virtual std::string getMcsEventFilePath() const = 0;

            virtual std::string getManagementAgentStatusCacheFilePath() const = 0;

            virtual std::string getLocalWarehouseRepository() const = 0;
            virtual std::string getLocalDistributionRepository() const = 0;

            /**
             * @brief gets the directory containing the symlinks to the plugin/product uninstallers <ProductLine>.sh for
             * each product <ProductLine> installed.
             * @return the full path of the directory
             */
            virtual std::string getLocalUninstallSymLinkPath() const = 0;

            virtual std::string getUpdateCertificatesPath() const = 0;
            virtual std::string getUpdateCacheCertificateFilePath() const = 0;
            virtual std::string getTempPath() const = 0;

            virtual std::string getSulDownloaderReportPath() const = 0;
            virtual std::string getSulDownloaderConfigFilePath() const = 0;
            virtual std::string getSulDownloaderReportGeneratedFilePath() const = 0;
            virtual std::string getSulDownloaderLockFilePath() const = 0;

            virtual std::string getSavedEnvironmentProxyFilePath() const = 0;
        };

        IApplicationPathManager& applicationPathManager();
        /** Use only for test */
        void replaceApplicationPathManager(std::unique_ptr<IApplicationPathManager>);
        void restoreApplicationPathManager();

    } // namespace ApplicationConfiguration
} // namespace Common
