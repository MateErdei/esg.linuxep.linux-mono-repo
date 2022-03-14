/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "SDDS3Repository.h"

#include "Sdds3Wrapper.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <CommsComponent/CommsComponentUtils.h>
#include <SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <SulDownloader/suldownloaderdata/Logger.h>
#include <SulDownloader/suldownloaderdata/ProductSelection.h>
#include <experimental/filesystem>

#include <Config.h>
#include <PackageRef.h>
#include <SyncLogic.h>
#include <iostream>
#include <json.hpp>
#include <utility>
#include <filesystem>


using namespace SulDownloader::suldownloaderdata;

constexpr const std::string_view SDDS3_DEFAULT_SERVICE_URL{ "https://sus.sophosupd.com" };
constexpr const std::array<std::string_view, 2> SDDS3_DEFAULT_CDN_URLS{
    "https://sdds3.sophosupd.com:443",
    "https://sdds3.sophosupd.net:443",
};

class applicationPathManager;
class applicationPathManager;
namespace SulDownloader
{
    SDDS3Repository::SDDS3Repository(const std::string& repo_dir, const std::string& certs_dir)
        : m_session(std::make_shared<sdds3::Session>(certs_dir))
        , m_repo(repo_dir)
        , m_supplementOnly(false)
    {
    }
    SDDS3Repository::SDDS3Repository()
        : m_session(std::make_shared<sdds3::Session>(""))
        , m_repo("")
        , m_supplementOnly(false)
    {}
    SDDS3Repository::~SDDS3Repository()
    {
    }

    bool SDDS3Repository::tryConnect(
        const suldownloaderdata::ConnectionSetup& connectionSetup,
        bool supplementOnly,
        const suldownloaderdata::ConfigurationData& configurationData)
    {
        m_supplementOnly = supplementOnly;
        populateOldConfigFromFile();

        m_dataToSync = getDataToSync(connectionSetup, configurationData);

        if(m_error.Description.empty())
        {
            return true;
        }

        return false;
    }

    bool SDDS3Repository::hasError() const
    {
        if(!m_error.Description.empty())
        {
            return true;
        }
        for(const auto& product : m_products)
        {
            if(product.hasError())
            {
                return true;
            }
        }
        return false;
    }

    bool SDDS3Repository::hasImmediateFailError() const
    {
        if (!hasError())
        {
            return false;
        }
        return m_error.status == PACKAGESOURCEMISSING;
    }

    RepositoryError SDDS3Repository::getError() const { return m_error; }

    std::vector<DownloadedProduct> SDDS3Repository::getProducts() const
    {
        return m_products;
    }

    std::string SDDS3Repository::getSourceURL() const { return {}; }

    std::vector<SubscriptionInfo> SDDS3Repository::listInstalledSubscriptions() const
    {
        return m_selectedSubscriptions;
    }

    std::string SDDS3Repository::getProductDistributionPath(const DownloadedProduct& product) const
    {
        std::string rootDistributionPath = Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository();
        return Common::FileSystem::join(rootDistributionPath, product.getLine());
    }

    std::vector<ProductInfo> SDDS3Repository::listInstalledProducts() const
    {
        return CatalogueInfo::calculatedListProducts(getProducts(), m_catalogueInfo);
    }

    void SDDS3Repository::purge() const
    {
        sdds3::purge(*m_session.get(), m_repo, m_config, m_oldConfig);
    }

    std::string writeSUSRequest(const SUSRequestParameters& parameters)
    {
        nlohmann::json json;
        json["schema_version"] = 1;
        json["product"] = parameters.product;
        json["server"] = parameters.isServer;
        json["platform_token"] = parameters.platformToken;
        json["subscriptions"] = nlohmann::json::array();
        for (const auto& subscription : parameters.subscriptions)
        {
            // TODO handle fixed version
            json["subscriptions"].push_back({ { "id", subscription.rigidName() }, { "tag", subscription.tag() } });
        }

        return json.dump();
    }

    void SDDS3Repository::parseSUSResponse(
        const std::string& response,
        std::set<std::string>& suites,
        std::set<std::string>& releaseGroups)
    {
        auto json = nlohmann::json::parse(response);

        if (json.contains("suites"))
        {
            for (const auto& s : json["suites"].items())
            {
                suites.insert(std::string(s.value()));
            }
        }

        if (json.contains("release-groups"))
        {
            for (const auto& g : json["release-groups"].items())
            {
                releaseGroups.insert(std::string(g.value()));
            }
        }
    }

    std::pair<std::set<std::string>, std::set<std::string>>
    SDDS3Repository::getDataToSync(const suldownloaderdata::ConnectionSetup& connectionSetup,
                                   const ConfigurationData& configurationData)
    {
        std::set<std::string> suites;
        std::set<std::string> releaseGroups;
        try
        {
            SUSRequestParameters requestParameters;
            requestParameters.product = "linuxep";
            requestParameters.platformToken = "LINUX_INTEL_LIBC6";

            requestParameters.subscriptions = configurationData.getProductsSubscription();
            requestParameters.subscriptions.insert(requestParameters.subscriptions.begin(),
                                                    configurationData.getPrimarySubscription());

            std::string request_json = writeSUSRequest(requestParameters);
            LOGDEBUG(request_json);

            // start of SUS request
            std::string userAgent = generateUserAgentString(configurationData.getTenantId(),
                                                            configurationData.getDeviceId());
            std::string overrideFile =
                Common::ApplicationConfiguration::applicationPathManager().getSdds3OverrideSettingsFile();
            bool useHttps = true;
            if(Common::FileSystem::fileSystem()->exists(overrideFile))
            {
                useHttps = (Common::UtilityImpl::StringUtils::extractValueFromIniFile(overrideFile, "USE_HTTP").empty());
            }

            auto http_session = std::make_unique<utilities::http::Session>(userAgent, utilities::http::Proxy(), useHttps);

            http_session->SetTimeouts(DEFAULT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS);

            //std::string url = DEFAULT_SERVICE_URL + "/v3/" + configurationData.getTenantId() + "/" + configurationData.getDeviceId();
            std::string url = connectionSetup.getUpdateLocationURL() + "/v3/" + configurationData.getTenantId() + "/" + configurationData.getDeviceId();
            auto http_connection = std::make_unique<utilities::http::Connection>(*http_session, url);

            auto request = std::make_unique<utilities::http::Request>(*http_connection, "POST", "");
            request->AddRequestHeader_Authorization("Bearer", configurationData.getJWToken());
            request->AddRequestHeader_ContentType("application/json");
            request->SetRequestPayload(request_json);
            unsigned int status_code = request->Send();
            if (status_code != 200)
            {
                std::stringstream message;
                message << "received HTTP response code " << status_code;
                m_error.Description = message.str();
                m_error.status = RepositoryStatus::CONNECTIONERROR;
                return std::make_pair(suites, releaseGroups);
            }

            parseSUSResponse(request->Read(), suites, releaseGroups);
            return std::make_pair(suites, releaseGroups);

        }
        catch (const std::exception& ex)
        {
            m_error.status = RepositoryStatus::CONNECTIONERROR;
            m_error.Description = ex.what();
            return std::make_pair(suites, releaseGroups);
        }
    }

    std::string SDDS3Repository::generateUserAgentString(const std::string& tenant_id, const std::string& device_id)
    {
        return "SophosUpdate/SDDS/3.0 (t=\"" + tenant_id + "\" d=\"" + device_id + "\" os=\"Linux\")";
    }

    void SDDS3Repository::synchronize(const suldownloaderdata::ConfigurationData& configurationData)
    {
        if(hasError())
        {
            return;
        }

        std::set<std::string>& suites = m_dataToSync.first;
        std::set<std::string>& releaseGroups = m_dataToSync.second;

        for (const auto& suite : suites)
        {
            LOGINFO("Suite: '" << suite << "' is available to be downloaded." );
        }

        for (const auto& releaseGroup : releaseGroups)
        {
            LOGINFO("Release Group: '" << releaseGroup << "' is available to be downloaded." );
        }

        //sdds3::Session session(CommsComponent::getCaCertificateStorePaths()[0]);
        m_session.reset();
        m_session = std::make_shared<sdds3::Session>("/etc/ssl/certs/ca-certificates.crt");
        //sdds3::Session session("/etc/ssl/certs/ca-certificates.crt");
        //m_session = session;
        sdds3::Repo repo(Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository());
        m_repo = repo;

        m_session->httpConfig.userAgent = generateUserAgentString(configurationData.getTenantId(), configurationData.getDeviceId());
        m_config.suites = suites;
        m_config.release_groups_filter = releaseGroups;

        try
        {
            std::string srcUrl = DEFAULT_UPDATE_URL; //configurationData.getSophosUpdateUrls()[0];
            SulDownloader::sdds3Wrapper()->sync(*m_session.get(), repo, srcUrl, m_config, m_oldConfig);

            if(!hasError())
            {
                generateProductListFromSdds3PackageInfo();
            }
        }
        catch (const std::exception& ex)
        {
            m_error.status = RepositoryStatus::DOWNLOADFAILED;
            std::stringstream message;
            message << "Faild to sync " << ex.what();
            m_error.Description = message.str();
            return;
        }
    }

    void SDDS3Repository::populateOldConfigFromFile()
    {
        std::string configFilePathString =
            Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath();
        try
        {
            if (Common::FileSystem::fileSystem()->exists(configFilePathString))
            {
                m_oldConfig =
                    SulDownloader::sdds3Wrapper()->loadConfig(configFilePathString);

                LOGINFO("Successfully loaded previous config file");
            }
            else
            {
                LOGINFO("Failed to find config file: " << configFilePathString);
            }
        }
        catch(const std::exception& ex)
        {
            LOGERROR("Failed to read previous SDDS3 config file, error:" << ex.what());
        }
    }

    void SDDS3Repository::generateProductListFromSdds3PackageInfo()
    {

        std::string configFilePathString =
            Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath();
        try
        {
            SulDownloader::sdds3Wrapper()->saveConfig(m_config, configFilePathString);
        }
        catch(const std::exception& ex)
        {
           LOGERROR("Failed to store SDDS3 config file, error:" << ex.what());
        }

        std::vector<sdds3::PackageRef> packagesWithSupplements;
        if(m_supplementOnly)
        {
            packagesWithSupplements =
                SulDownloader::sdds3Wrapper()->getPackagesIncludingSupplements(*m_session.get(), m_repo, m_config);
        }
        std::vector<sdds3::PackageRef> packagesToInstall =
            SulDownloader::sdds3Wrapper()->getPackagesToInstall(*m_session.get(), m_repo, m_config, m_oldConfig);
        std::vector<sdds3::PackageRef> allPackages =
            SulDownloader::sdds3Wrapper()->getPackages(*m_session.get(), m_repo, m_config);

        m_selectedSubscriptions.clear();

        std::vector<sdds3::PackageRef> packagesOfInterest;

        if (m_supplementOnly)
        {
            packagesOfInterest = packagesWithSupplements;
        }
        else
        {
            packagesOfInterest = allPackages;
        }

        for (auto& package : allPackages)
        {
            if (package.lineId_ == "ServerProtectionLinux-Base")
            {
                continue;
            }
            ProductMetadata productMetadata;
            productMetadata.setLine(package.lineId_);
            productMetadata.setDefaultHomePath(package.decodePath_);
            productMetadata.setName(package.name_);
            productMetadata.setFeatures(package.features_);

            std::vector<Tag> tags;

            for(auto& releaseTag : package.tags_)
            {
                tags.emplace_back("", "", releaseTag.name);
            }
            productMetadata.setTags(tags);
            productMetadata.setVersion(package.version_);

            DownloadedProduct product(productMetadata);

            product.setProductHasChanged(false);

            for (auto& packageToInstall : packagesToInstall)
            {
                if (packageToInstall.lineId_ == package.lineId_)
                {
                    product.setProductHasChanged(true);
                    break;
                }
            }

            if(m_supplementOnly)
            {
                bool packageContainsSupplement = false;
                for (auto& packageWithsupplement : packagesWithSupplements)
                {
                    if (packageWithsupplement.lineId_ == package.lineId_)
                    {
                        packageContainsSupplement = true;
                        break;
                    }
                }
                product.setProductHasChanged(product.productHasChanged() && packageContainsSupplement);
            }

            m_selectedSubscriptions.push_back(
                { productMetadata.getLine(), productMetadata.getVersion(), productMetadata.subProducts() });

            product.setDistributePath(
                       Common::FileSystem::join(
                        Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository(),
                        package.lineId_));

            m_products.push_back(product);
        }
    }

    void SDDS3Repository::distribute()
    {
        if(hasError())
        {
            return;
        }
        try
        {
            SulDownloader::sdds3Wrapper()->extractPackagesTo(*m_session.get(), m_repo, m_config, Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository());
        }
        catch (...)
        {
            m_error.status = RepositoryStatus::DOWNLOADFAILED;
            m_error.Description = "Failed to extract packages";
        }

    }

} // namespace SulDownloader
