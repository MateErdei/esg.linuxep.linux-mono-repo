/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "SDDS3Repository.h"

using namespace SulDownloader::suldownloaderdata;

constexpr const std::string_view SDDS3_DEFAULT_SERVICE_URL{ "https://sus.sophosupd.com" };
constexpr const std::array<std::string_view, 2> SDDS3_DEFAULT_CDN_URLS{
    "https://sdds3.sophosupd.com",
    "https://sdds3.sophosupd.net",
};

namespace SulDownloader
{
    SDDS3Repository::SDDS3Repository(const std::string& repo_dir, const std::string& certs_dir) : m_session(certs_dir), m_repo(repo_dir)
    {
    }

    SDDS3Repository::~SDDS3Repository() {}

    bool SDDS3Repository::hasError() const { return false; }

    bool SDDS3Repository::hasImmediateFailError() const { return false; }

    RepositoryError SDDS3Repository::getError() const { return {}; }

    std::vector<DownloadedProduct> SDDS3Repository::getProducts() const { return {}; }

    std::string SDDS3Repository::getSourceURL() const { return {}; }

    std::vector<SubscriptionInfo> SDDS3Repository::listInstalledSubscriptions() const { return {}; }

    std::string SDDS3Repository::getProductDistributionPath(const DownloadedProduct&) const { return {}; }

    std::vector<ProductInfo> SDDS3Repository::listInstalledProducts() const { return {}; }

    void SDDS3Repository::purge() const {}

    void SDDS3Repository::sync(const std::string& src_url, sdds3::Config& config)
    {
        sdds3::sync(m_session, m_repo, src_url, config);
    }
} // namespace SulDownloader
