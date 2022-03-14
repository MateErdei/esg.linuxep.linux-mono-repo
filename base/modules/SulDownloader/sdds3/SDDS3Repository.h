/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "Sdds3Wrapper.h"
#include <SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/ISdds3Repository.h>
#include <SulDownloader/suldownloaderdata/Tag.h>
#include <SulDownloader/suldownloaderdata/ConnectionSetup.h>

#include <SyncLogic.h>


namespace SulDownloader
{

//    static const std::string DEFAULT_SERVICE_URL = "https://sus.sophosupd.com";
//    static const std::string DEFAULT_UPDATE_URL = "https://sdds3.sophosupd.com";
    static const std::string DEFAULT_SERVICE_URL = "http://127.0.0.1:8080";
    static const std::string DEFAULT_UPDATE_URL = "http://127.0.0.1:8080";
    static const int DEFAULT_TIMEOUT_MS = 5000;

    struct SUSRequestParameters
    {
        std::string product;
        bool isServer = false;
        std::string platformToken;
        std::vector<suldownloaderdata::ProductSubscription> subscriptions;
    };

    class SDDS3Repository : public virtual suldownloaderdata::ISdds3Repository
    {
    public:
        SDDS3Repository(const std::string& repo_dir, const std::string& certs_dir);

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
        //void setSdds3WrapperInstance(std::shared_ptr<ISdds3Wrapper> sdds3Wrapper);

        void generateProductListFromSdds3PackageInfo();

    private:
        void populateOldConfigFromFile();
        std::pair<std::set<std::string>, std::set<std::string>>
        getDataToSync( const suldownloaderdata::ConnectionSetup& connectionSetup,
                      const suldownloaderdata::ConfigurationData& configurationData);
        void parseSUSResponse(
            const std::string& response,
            std::set<std::string>& suites,
            std::set<std::string>& releaseGroups);
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
        std::pair<std::set<std::string>, std::set<std::string>> m_dataToSync;
        //std::shared_ptr<ISdds3Wrapper> m_sdds3Wrapper;
    };
} // namespace SulDownloader
