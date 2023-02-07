/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "SDDS3Repository.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <SulDownloader/suldownloaderdata/Logger.h>
#include "CurlWrapper.h"
#include "ICurlWrapper.h"

#include <Config.h>
#include <PackageRef.h>
#include "Sdds3Wrapper.h"
#include "SusRequestParameters.h"
#include "HttpRequestsImpl/HttpRequesterImpl.h"
#include "UpdateUtilities/InstalledFeatures.h"

#include <sophlib/logging/Logging.h>

#include <iostream>

using namespace SulDownloader::suldownloaderdata;

constexpr const std::string_view SDDS3_DEFAULT_SERVICE_URL{ "https://sus.sophosupd.com" };
constexpr const std::array<std::string_view, 2> SDDS3_DEFAULT_CDN_URLS{
    "https://sdds3.sophosupd.com:443",
    "https://sdds3.sophosupd.net:443",
};

class applicationPathManager;

namespace SulDownloader
{
    SDDS3Repository::SDDS3Repository(const std::string& repoDir, const std::string& certsDir)
        : m_session(std::make_shared<sdds3::Session>(std::vector<std::filesystem::path>{certsDir}))
        , m_repo(repoDir)
        , m_supplementOnly(false)
    {
        setupSdds3LibLogger();
    }
    SDDS3Repository::SDDS3Repository()
        : m_session(std::make_shared<sdds3::Session>(std::vector<std::filesystem::path>{}))
        , m_repo("")
        , m_supplementOnly(false)
    {
        setupSdds3LibLogger();
    }
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
        setFeatures(configurationData.getFeatures());
        m_dataToSync = getDataToSync(connectionSetup, configurationData);

        if (m_error.Description.empty())
        {
            return true;
        }
        LOGDEBUG("Getting suites failed with: " << m_error.Description);
        return false;
    }

    bool SDDS3Repository::doesFeatureCodeMatchConfig(const std::vector<std::string>& keys)
    {
        if (!m_configFeatures.empty() && !keys.empty())
        {
            for (auto const& key : keys)
            {
                if (std::count(m_configFeatures.begin(), m_configFeatures.end(), key))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool SDDS3Repository::hasError() const
    {
        if (!m_error.Description.empty())
        {
            return true;
        }
        for (const auto& product : m_products)
        {
            if (product.hasError())
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

    std::string SDDS3Repository::getSourceURL() const
    {
        return m_sourceUrl;
    }

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

    void SDDS3Repository::setupSdds3LibLogger()
    {
        sophlib::logging::LogLevel log_level = sophlib::logging::LogLevel::Debug;
        sophlib::logging::GlobalLogger().SetupSingleFile(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderSynLogPath(), log_level);
    }

    void SDDS3Repository::setFeatures(const std::vector<std::string>& configfeatures)
    {
        m_configFeatures = configfeatures;
    }


    SDDS3::SusData SDDS3Repository::getDataToSync(const suldownloaderdata::ConnectionSetup& connectionSetup,
                                   const ConfigurationData& configurationData)
    {
        SDDS3::SusData susData;
        try
        {
            SUSRequestParameters requestParameters;
            requestParameters.product = "linuxep";
            requestParameters.platformToken = "LINUX_INTEL_LIBC6";
            requestParameters.subscriptions = configurationData.getProductsSubscription();
            requestParameters.subscriptions.insert(requestParameters.subscriptions.begin(),
                                                    configurationData.getPrimarySubscription());
            requestParameters.tenantId = configurationData.getTenantId();
            requestParameters.deviceId = configurationData.getDeviceId();
            requestParameters.baseUrl = connectionSetup.getUpdateLocationURL();
            requestParameters.jwt = configurationData.getJWToken();
            requestParameters.timeoutSeconds = DEFAULT_TIMEOUT_S;
            requestParameters.proxy = connectionSetup.getProxy();

            std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curl = std::make_shared<Common::CurlWrapper::CurlWrapper>();
            std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curl);
            SDDS3::SusRequester susRequester(httpClient);

            auto susResponse = susRequester.request(requestParameters);
            if (susResponse.success)
            {
                LOGINFO("SUS Request was successful");
                m_error.status = RepositoryStatus::SUCCESS;
                m_error.Description = "";
                checkForMissingPackages(requestParameters.subscriptions, susResponse.data.suites);
                susData = susResponse.data;
            }
            else
            {
                m_error.status = RepositoryStatus::CONNECTIONERROR;
                m_error.Description = susResponse.error;
            }
        }
        catch (const std::exception& ex)
        {
            m_error.status = RepositoryStatus::CONNECTIONERROR;
            m_error.Description = ex.what();
        }
        return susData;
    }

    void SDDS3Repository::checkForMissingPackages(const std::vector<ProductSubscription>& subscriptions,const std::set<std::string>& suites)
    {
        LOGDEBUG("Checking for missing packages");
        size_t prefixSize = std::string("sdds3.").size();
        for (const auto& sub: subscriptions)
        {
            const std::string& productID = sub.rigidName();

            bool notFound = (std::find_if ( suites.begin(), suites.end(),
                                          [productID,prefixSize](const std::string& suiteName)
                                          {
                                            std::string warehouseSuite = suiteName.substr(prefixSize, suiteName.size()-prefixSize);
                                            return Common::UtilityImpl::StringUtils::startswith(warehouseSuite, productID);
                                          }
            )== suites.end());

            if (notFound)
            {
                std::stringstream errorMessage;
                errorMessage << "Package : " << productID << " missing from warehouse";
                m_error.Description = errorMessage.str();
                m_error.status = RepositoryStatus::PACKAGESOURCEMISSING;
                break;
            }
        }
    }

    std::string SDDS3Repository::generateUserAgentString(const std::string& tenant_id, const std::string& device_id)
    {
        return "SophosUpdate/SDDS/3.0 (t=\"" + tenant_id + "\" d=\"" + device_id + "\" os=\"Linux\")";
    }

    bool SDDS3Repository::synchronize(
        const suldownloaderdata::ConfigurationData& configurationData,
        const suldownloaderdata::ConnectionSetup& connectionSetup,
        const bool ignoreFailedSupplementRefresh)
    {
        m_error.reset(); // Clear error for retry

        std::set<std::string>& suites = m_dataToSync.suites;
        std::set<std::string>& releaseGroups = m_dataToSync.releaseGroups;

        for (const auto& suite : suites)
        {
            LOGINFO("Suite: '" << suite << "' is available to be downloaded." );
        }

        for (const auto& releaseGroup : releaseGroups)
        {
            LOGDEBUG("Release Group: '" << releaseGroup << "' is available to be downloaded." );
        }

        m_session = std::make_shared<sdds3::Session>(std::vector<std::filesystem::path>{
            Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath()});
        std::string srcUrl = connectionSetup.getUpdateLocationURL();

        m_session->httpConfig.connectTimeoutMs = 10000;
        if (connectionSetup.isCacheUpdate())
        {
            if (!Common::UtilityImpl::StringUtils::startswith(srcUrl, "http://") &&
                !Common::UtilityImpl::StringUtils::startswith(srcUrl, "https://"))
            {
                srcUrl = "https://" + srcUrl;
            }
            srcUrl = srcUrl+ "/v3";
            LOGINFO("Trying update via update cache: " << srcUrl);
            m_session->httpConfig.useSophosCertificateStore = true;
            m_session->httpConfig.useUpdateCache = true;
            if (configurationData.getUpdateCacheCertPath().empty())
            {
                m_session->httpConfig.sophosCertificateStore = Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
            }
            else
            {
                m_session->httpConfig.sophosCertificateStore = configurationData.getUpdateCacheCertPath();
            }

        }
        else if (!connectionSetup.getProxy().empty())
        {
            m_session->httpConfig.proxy.address_ = connectionSetup.getProxy().getUrl();
            std::stringstream message;
            message << "Trying to update via proxy " << connectionSetup.getProxy().getUrl() << " to " << srcUrl;
            LOGINFO(message.str());
            m_session->httpConfig.proxy.username_ = connectionSetup.getProxy().getCredentials().getUsername();
            m_session->httpConfig.proxy.password_ = connectionSetup.getProxy().getCredentials().getPassword();
        }
        else
        {
            LOGINFO("Connecting to update source directly");
        }

        sdds3::Repo repo(Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository());
        m_repo = repo;

        m_session->httpConfig.userAgent = generateUserAgentString(configurationData.getTenantId(), configurationData.getDeviceId());

        m_config.sus_response.suites = suites;
        m_config.sus_response.release_groups_filter = releaseGroups;

        try
        {
            LOGINFO("Performing Sync using " << srcUrl);
            SulDownloader::sdds3Wrapper()->sync(*m_session.get(), repo, srcUrl, m_config, m_oldConfig);
            m_sourceUrl = srcUrl;  // store which source was used - for reporting later.
            if (!ignoreFailedSupplementRefresh)
            {
                // supplementFallbackCount is intialized then the sync call failed to pull down the latest supplements
                if (m_session->syncStats.supplementFallbackCount)
                {
                    m_error.status = RepositoryStatus::DOWNLOADFAILED;
                    std::string message;
                    message = "Failed to sync supplements";
                    m_error.Description = message;
                    LOGWARN("Failed to Sync with " << srcUrl << " error: " << message);
                }
            }
        }
        catch (const std::exception& ex)
        {
            m_error.status = RepositoryStatus::DOWNLOADFAILED;
            std::stringstream message;
            message << "Failed to sync " << ex.what();
            m_error.Description = message.str();
            LOGWARN("Failed to Sync with " << srcUrl << " error: " << ex.what());
        }

        if (!hasError())
        {
            generateProductListFromSdds3PackageInfo(configurationData.getPrimarySubscription().rigidName());
            return true;
        }
        return false;
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

    void SDDS3Repository::generateProductListFromSdds3PackageInfo(const std::string& primaryRigidName)
    {
        std::vector<sdds3::PackageRef> packagesWithSupplements;
        if (m_supplementOnly)
        {
            packagesWithSupplements =
                SulDownloader::sdds3Wrapper()->getPackagesIncludingSupplements(*m_session.get(), m_repo, m_config);
        }
        m_oldConfig.platform_filter = std::nullopt;
        std::vector<sdds3::PackageRef> packagesToInstall =
            SulDownloader::sdds3Wrapper()->getPackagesToInstall(*m_session.get(), m_repo, m_config, m_oldConfig);

        std::string configFilePathString =
                Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath();
        try
        {
            // save config to disk after getPackagesToInstall, which generates package thumbprints for m_config
            SulDownloader::sdds3Wrapper()->saveConfig(m_config, configFilePathString);
        }
        catch(const std::exception& ex)
        {
            LOGERROR("Failed to store SDDS3 config file, error:" << ex.what());
        }

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

        // Get the currently installed features. We need to make sure if the required feature list changes
        // and we have a downloaded component that is not yet installed but is added to the required list then
        // we do actually run the installer by marking the product as changed.
        std::vector<std::string> installedFeatures;
        if (Common::UpdateUtilities::doesInstalledFeaturesListExist())
        {
            try
            {
                installedFeatures = Common::UpdateUtilities::readInstalledFeaturesJsonFile();
            }
            catch (const std::exception& exception)
            {
                LOGERROR("Failed to read currently installed feature list");
            }
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
            m_catalogueInfo.addInfo(package.lineId_, package.version_, package.name_);

            std::vector<Tag> tags;

            for (auto& releaseTag : package.tags_)
            {
                tags.emplace_back("", "", releaseTag.name);
            }
            productMetadata.setTags(tags);
            productMetadata.setVersion(package.version_);
            productMetadata.setName(package.name_);
            DownloadedProduct product(productMetadata);

            product.setProductHasChanged(false);

            for (auto& packageToInstall : packagesToInstall)
            {
                if (packageToInstall.lineId_ == package.lineId_)
                {
                    product.setProductHasChanged(true);
                    m_willInstall = true;
                    break;
                }
            }

            if (Common::UpdateUtilities::shouldProductBeInstalledBasedOnFeatures(productMetadata.getFeatures(), installedFeatures, m_configFeatures))
            {
                LOGDEBUG("Already downloaded product requires install: " << product.getLine());
                product.setProductHasChanged(true);
                m_willInstall = true;
            }

            if (m_supplementOnly)
            {
                bool packageContainsSupplement = false;
                for (auto& packageWithSupplement : packagesWithSupplements)
                {
                    if (packageWithSupplement.lineId_ == package.lineId_)
                    {
                        packageContainsSupplement = true;
                        break;
                    }
                }
                if (product.productHasChanged() && packageContainsSupplement)
                {
                    product.setProductHasChanged(true);
                    m_willInstall = true;
                }
                else
                {
                    product.setProductHasChanged(false);
                }
            }

            // Don't install features we don't want
            if (!doesFeatureCodeMatchConfig(productMetadata.getFeatures()))
            {
                continue;
            }

            product.setDistributePath(
                       Common::FileSystem::join(
                        Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository(),
                        package.lineId_));

            // ensure that the primary product is the first in the list,
            // so that primary product always being installed first.
            if (product.getLine().find(primaryRigidName) != std::string::npos)
            {
                m_products.insert(m_products.begin(), product);
            }
            else
            {
                m_products.push_back(product);
            }
        }

        // Add suites details to the subscription list
        std::vector<sdds3::Suite> suites = SulDownloader::sdds3Wrapper()->getSuites(*m_session.get(), m_repo, m_config);
        for(auto& suite : suites)
        {
            m_selectedSubscriptions.push_back(
                {suite.name_, suite.marketing_version_, {}});
        }
    }

    void SDDS3Repository::setWillInstall(const bool willInstall)
    {
        m_willInstall = willInstall;
    }

    void SDDS3Repository::distribute()
    {
        if (hasError()  || !m_willInstall)
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
