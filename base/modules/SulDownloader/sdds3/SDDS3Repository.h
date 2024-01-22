// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISusRequester.h"
#include "Sdds3Wrapper.h"

#include "SulDownloader/suldownloaderdata/ConnectionSetup.h"
#include "SulDownloader/suldownloaderdata/DownloadedProduct.h"
#include "SulDownloader/suldownloaderdata/ISdds3Repository.h"
#include "SulDownloader/suldownloaderdata/Tag.h"

namespace SulDownloader
{
    static const int DEFAULT_TIMEOUT_S = 60;

    class SDDS3Repository : public virtual suldownloaderdata::ISdds3Repository
    {
    public:
        SDDS3Repository(
            std::unique_ptr<SDDS3::ISusRequester> susRequester,
            const std::string& repoDir,
            const std::string& certsDir);
        explicit SDDS3Repository(std::unique_ptr<SDDS3::ISusRequester> susRequester);

        ~SDDS3Repository() override;

        bool tryConnect(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            bool supplementOnly,
            const Common::Policy::UpdateSettings& configurationData) override;
        bool hasError() const override;
        bool hasImmediateFailError() const override;
        suldownloaderdata::RepositoryError getError() const override;
        std::vector<suldownloaderdata::DownloadedProduct> getProducts() const override;
        std::string getSourceURL() const override;
        std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const override;
        std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const override;
        std::vector<Common::DownloadReport::ProductInfo> listInstalledProducts() const override;
        void purge() const override;
        bool synchronize(
            const Common::Policy::UpdateSettings& configurationData,
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            const bool ignoreFailedSupplementRefresh) override;
        void distribute() override;
        void setDoUnpackRepository(const bool willInstall) override;
        /***
         * Updates the m_product 'vector' using the primaryRigidName value to determine
         * Which product should be first in the list.
         * @param primaryRigidName obtained from configuration data.
         */
        void generateProductListFromSdds3PackageInfo(const std::string& primaryRigidName);
        void checkForMissingPackages(
            const std::vector<Common::Policy::ProductSubscription>& subscriptions,
            const std::set<std::string>& suites);

        void setFeatures(const std::vector<std::string>& configfeatures);

    private:
        static void writeToIni(bool usedProxy, bool usedUpdateCache);
        void setupSdds3LibLogger();
        void populateConfigFromFile();
        SDDS3::SusData getDataToSync(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            const Common::Policy::UpdateSettings& configurationData);
        bool doesFeatureCodeMatchConfig(const std::vector<std::string>& keys);
        std::string generateUserAgentString(const std::string& tenant_id, const std::string& device_id);
        std::shared_ptr<sophlib::sdds3::Session> m_session;
        sophlib::sdds3::Repo m_repo;
        sophlib::sdds3::Config m_config;
        sophlib::sdds3::Config m_oldConfig;
        std::vector<suldownloaderdata::DownloadedProduct> m_products;
        suldownloaderdata::RepositoryError m_error;
        std::vector<suldownloaderdata::SubscriptionInfo> m_selectedSubscriptions;
        bool m_supplementOnly;
        std::string m_sourceUrl;
        std::vector<std::string> m_configFeatures;
        bool m_willInstall = false;
        std::unique_ptr<SDDS3::ISusRequester> susRequester_;
    };
} // namespace SulDownloader
