/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <SulDownloader/suldownloaderdata/DownloadedProduct.h>
#include <SulDownloader/suldownloaderdata/IRepository.h>
#include <SulDownloader/suldownloaderdata/Tag.h>

#include <SyncLogic.h>

namespace SulDownloader
{
    class SDDS3Repository : public virtual suldownloaderdata::IRepository
    {
    public:
        SDDS3Repository(const std::string& repo_dir, const std::string& certs_dir);
        ~SDDS3Repository();

        bool hasError() const override;
        bool hasImmediateFailError() const override;
        suldownloaderdata::RepositoryError getError() const override;
        std::vector<suldownloaderdata::DownloadedProduct> getProducts() const override;
        std::string getSourceURL() const override;
        std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const override;
        std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const override;
        std::vector<suldownloaderdata::ProductInfo> listInstalledProducts() const override;
        void purge() const override;

        void sync(const std::string& src_url, sdds3::Config& config);

    private:
        sdds3::Session m_session;
        sdds3::Repo m_repo;
        sdds3::Config m_config;
    };
} // namespace SulDownloader
