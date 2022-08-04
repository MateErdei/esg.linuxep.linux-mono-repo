/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "Sdds3Wrapper.h"
#include "SusRequester.h"

#include <Common/HttpRequests/IHttpRequester.h>
#include <SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <SulDownloader/suldownloaderdata/ConnectionSetup.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ISdds3Repository.h>
#include <SulDownloader/suldownloaderdata/Tag.h>

namespace SulDownloader
{
    static const std::string DEFAULT_SERVICE_URL = "https://sus.sophosupd.com";
    static const std::vector<std::string> DEFAULT_UPDATE_URLS = { "https://sdds3.sophosupd.com:443",
                                                                  "https://sdds3.sophosupd.net:443" };
    static const int DEFAULT_TIMEOUT_S = 5;

    class SDDS3Repository : public virtual suldownloaderdata::ISdds3Repository
    {
    public:
        SDDS3Repository(const std::string& repoDir, const std::string& certsDir);

        ~SDDS3Repository();

        bool tryConnect(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            bool supplementOnly,
            const suldownloaderdata::ConfigurationData& configurationData) override;
        bool hasError() const override;
        bool hasImmediateFailError() const override;
        suldownloaderdata::RepositoryError getError() const override;
        std::vector<suldownloaderdata::DownloadedProduct> getProducts() const override;
        std::string getSourceURL() const override;
        std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const override;
        std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const override;
        std::vector<suldownloaderdata::ProductInfo> listInstalledProducts() const override;
        void purge() const override;
        SDDS3Repository();
        void synchronize(const suldownloaderdata::ConfigurationData& configurationData) override;
        void distribute() override;

        /***
         * Updates the m_product 'vector' using the primaryRigidName value to determine
         * Which product should be first in the list.
         * @param primaryRigidName obtained from configuration data.
         */
        void generateProductListFromSdds3PackageInfo(const std::string& primaryRigidName);
        void checkForMissingPackages(
            const std::vector<suldownloaderdata::ProductSubscription>& subscriptions,
            const std::set<std::string>& suites);

        void setFeatures(const std::vector<std::string>& configfeatures);

    private:
        void setupSdds3LibLogger();
        void populateOldConfigFromFile();
        SDDS3::SusData getDataToSync(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            const suldownloaderdata::ConfigurationData& configurationData);
        bool doesFeatureCodeMatchConfig(const std::vector<std::string>& keys);
        std::string generateUserAgentString(const std::string& tenant_id, const std::string& device_id);
        std::shared_ptr<sdds3::Session> m_session;
        sdds3::Repo m_repo;
        sdds3::Config m_config;
        sdds3::Config m_oldConfig;
        std::vector<suldownloaderdata::DownloadedProduct> m_products;
        suldownloaderdata::RepositoryError m_error;
        suldownloaderdata::CatalogueInfo m_catalogueInfo;
        std::vector<suldownloaderdata::SubscriptionInfo> m_selectedSubscriptions;
        bool m_supplementOnly;
        SDDS3::SusData m_dataToSync;
        std::string m_sourceUrl;
        std::vector<std::string> m_configFeatures;
    };
} // namespace SulDownloader
