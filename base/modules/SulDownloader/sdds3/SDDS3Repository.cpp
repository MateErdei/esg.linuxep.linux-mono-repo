// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SDDS3Repository.h"

#include "Logger.h"
#include "Sdds3Wrapper.h"
#include "SusRequestParameters.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UpdateUtilities/InstalledFeatures.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "SulDownloader/suldownloaderdata/CatalogueInfo.h"
#include "sophlib/logging/Logging.h"
#include "sophlib/sdds3/Config.h"
#include "sophlib/sdds3/PackageRef.h"

#include <iostream>

using namespace Common::Policy;
using namespace SulDownloader::suldownloaderdata;
using namespace Common::DownloadReport;

class applicationPathManager;

namespace SulDownloader
{
    SDDS3Repository::SDDS3Repository(
        std::unique_ptr<SDDS3::ISusRequester> susRequester,
        const std::string& repoDir,
        const std::string& certsDir) :
        m_session(std::make_shared<sophlib::sdds3::Session>(std::vector<std::filesystem::path>{ certsDir })),
        m_repo(repoDir),
        m_supplementOnly(false),
        susRequester_(std::move(susRequester))
    {
        setupSdds3LibLogger();
    }
    // m_session and m_repo are re-initialised in synchronize()
    SDDS3Repository::SDDS3Repository(std::unique_ptr<SDDS3::ISusRequester> susRequester) :
        m_session(std::make_shared<sophlib::sdds3::Session>(std::vector<std::filesystem::path>{})),
        m_repo(""),
        m_supplementOnly(false),
        susRequester_(std::move(susRequester))
    {
        setupSdds3LibLogger();
    }
    SDDS3Repository::~SDDS3Repository() {}

    bool SDDS3Repository::tryConnect(
        const suldownloaderdata::ConnectionSetup& connectionSetup,
        bool supplementOnly,
        const UpdateSettings& updateSetting)
    {
        m_supplementOnly = supplementOnly;
        populateConfigFromFile();
        setFeatures(updateSetting.getFeatures());

        // If we are performing a supplement-only update, we want to avoid fetching and installing new non-supplement
        // packages. So, we should reuse the previous response from SUS if we have it.
        if (m_supplementOnly && !m_config.sus_response.suites.empty())
        {
            LOGINFO("Using previously acquired suites and release groups");
        }
        else
        {
            // If we are doing a supplement-only update, but don't already have a response from SUS, then it is an error
            // We should still carry on and do a product update (which includes supplements)
            if (m_supplementOnly)
            {
                LOGERROR("Supplement-only update requested but no cached SUS response present, doing product update");
            }

            LOGDEBUG("Checking suites and release groups with SUS");

            SDDS3::SusData susData = getDataToSync(connectionSetup, updateSetting);

            if (!m_error.Description.empty())
            {
                LOGDEBUG("Getting suites failed with: " << m_error.Description);
                return false;
            }

            m_config.sus_response.suites = susData.suites;
            m_config.sus_response.release_groups_filter = susData.releaseGroups;
        }

        return true;
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

    RepositoryError SDDS3Repository::getError() const
    {
        return m_error;
    }

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
        std::string rootDistributionPath =
            Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository();
        return Common::FileSystem::join(rootDistributionPath, product.getLine());
    }

    std::vector<ProductInfo> SDDS3Repository::listInstalledProducts() const
    {
        return CatalogueInfo::calculatedListProducts(getProducts(), m_catalogueInfo);
    }

    void SDDS3Repository::purge() const
    {
        sophlib::sdds3::Purge(*m_session.get(), m_repo, m_config, m_oldConfig);
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

    SDDS3::SusData SDDS3Repository::getDataToSync(
        const suldownloaderdata::ConnectionSetup& connectionSetup,
        const UpdateSettings& updateSettings)
    {
        SDDS3::SusData susData;
        try
        {
            SUSRequestParameters requestParameters;
            requestParameters.product = "linuxep";
            requestParameters.platformToken = Common::Policy::machineArchitecture_;
            requestParameters.subscriptions = updateSettings.getProductsSubscription();
            requestParameters.subscriptions.insert(
                requestParameters.subscriptions.begin(), updateSettings.getPrimarySubscription());
            requestParameters.tenantId = updateSettings.getTenantId();
            requestParameters.deviceId = updateSettings.getDeviceId();
            requestParameters.baseUrl = connectionSetup.getUpdateLocationURL();
            requestParameters.jwt = updateSettings.getJWToken();
            requestParameters.timeoutSeconds = DEFAULT_TIMEOUT_S;
            requestParameters.proxy = connectionSetup.getProxy();
            requestParameters.esmVersion = updateSettings.getEsmVersion();

            auto susResponse = susRequester_->request(requestParameters);
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
                if (!susResponse.persistentError)
                {
                    m_error.status = RepositoryStatus::CONNECTIONERROR;
                }
                else
                {
                    m_error.status = RepositoryStatus::DOWNLOADFAILED;
                }
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

    void SDDS3Repository::checkForMissingPackages(
        const std::vector<ProductSubscription>& subscriptions,
        const std::set<std::string>& suites)
    {
        LOGDEBUG("Checking for missing packages");
        const std::string prefix{ "sdds3." };
        for (const auto& sub : subscriptions)
        {
            const std::string& productID = sub.rigidName();

            bool notFound = std::find_if(
                                suites.begin(),
                                suites.end(),
                                [productID, prefix](const std::string& suiteName)
                                {
                                    if (!Common::UtilityImpl::StringUtils::startswith(suiteName, prefix))
                                    {
                                        return false;
                                    }
                                    std::string warehouseSuite =
                                        suiteName.substr(prefix.size(), suiteName.size() - prefix.size());
                                    return Common::UtilityImpl::StringUtils::startswith(warehouseSuite, productID);
                                }) == suites.end();

            if (notFound)
            {
                std::stringstream errorMessage;
                errorMessage << "Product doesn't match any suite: " << productID;
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
        const UpdateSettings& updateSetting,
        const suldownloaderdata::ConnectionSetup& connectionSetup,
        const bool ignoreFailedSupplementRefresh)
    {
        m_error.reset(); // Clear error for retry

        for (const auto& suite : m_config.sus_response.suites)
        {
            if (m_oldConfig.sus_response.suites.find(suite) == m_oldConfig.sus_response.suites.end())
            {
                LOGINFO("Suite: '" << suite << "' is available to be downloaded.");
            }
        }

        for (const auto& releaseGroup : m_config.sus_response.release_groups_filter)
        {
            if (m_oldConfig.sus_response.release_groups_filter.find(releaseGroup) ==
                m_oldConfig.sus_response.release_groups_filter.end())
            {
                LOGDEBUG("Release Group: '" << releaseGroup << "' is available to be downloaded.");
            }
        }

        m_session = std::make_shared<sophlib::sdds3::Session>(std::vector<std::filesystem::path>{
            Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath() });
        std::string srcUrl = connectionSetup.getUpdateLocationURL();

        m_session->httpConfig.connectTimeoutMs = 60000;

        if (updateSetting.getUseSdds3DeltaV2())
        {
            m_session->deltaVersioningEnabled = true;
            LOGDEBUG("Enabling sdds3 delta V2 usage");
        }
        else
        {
            m_session->deltaVersioningEnabled = false;
        }

        if (connectionSetup.isCacheUpdate())
        {
            if (!Common::UtilityImpl::StringUtils::startswith(srcUrl, "http://") &&
                !Common::UtilityImpl::StringUtils::startswith(srcUrl, "https://"))
            {
                srcUrl = "https://" + srcUrl;
            }
            srcUrl = srcUrl + "/v3";
            LOGINFO("Trying update via update cache: " << srcUrl);
            m_session->httpConfig.useSophosCertificateStore = true;
            m_session->httpConfig.useUpdateCache = true;
            if (updateSetting.getUpdateCacheCertPath().empty())
            {
                m_session->httpConfig.sophosCertificateStore =
                    Common::ApplicationConfiguration::applicationPathManager().getUpdateCacheCertificateFilePath();
            }
            else
            {
                m_session->httpConfig.sophosCertificateStore = updateSetting.getUpdateCacheCertPath();
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

        sophlib::sdds3::Repo repo(Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository());
        m_repo = repo;

        m_session->httpConfig.userAgent =
            generateUserAgentString(updateSetting.getTenantId(), updateSetting.getDeviceId());

        try
        {
            LOGINFO("Performing Sync using " << srcUrl);
            SulDownloader::sdds3Wrapper()->sync(*m_session.get(), repo, srcUrl, m_config, m_oldConfig);
            m_sourceUrl = srcUrl; // store which source was used - for reporting later.
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
            generateProductListFromSdds3PackageInfo(updateSetting.getPrimarySubscription().rigidName());
            return true;
        }
        return false;
    }

    void SDDS3Repository::populateConfigFromFile()
    {
        std::string configFilePathString =
            Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath();
        try
        {
            if (Common::FileSystem::fileSystem()->exists(configFilePathString))
            {
                m_oldConfig = SulDownloader::sdds3Wrapper()->loadConfig(configFilePathString);
                // Config load code sets platform_filter to an empty string even if it was saved without a value
                // Hence we need to unset it here
                m_oldConfig.platform_filter = std::nullopt;
                // Base the new config on the old config so that the suites are preserved from the last SUS request
                m_config = m_oldConfig;

                LOGINFO("Successfully loaded previous config file");
            }
            else
            {
                LOGINFO("Failed to find config file: " << configFilePathString);
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Failed to read previous SDDS3 config file, error:" << ex.what());
        }
    }

    void SDDS3Repository::generateProductListFromSdds3PackageInfo(const std::string& primaryRigidName)
    {
        // This call updates m_config with the current package thumbprints
        std::vector<sophlib::sdds3::PackageRef> packagesToInstall =
            SulDownloader::sdds3Wrapper()->getPackagesToInstall(*m_session.get(), m_repo, m_config, m_oldConfig);

        std::string configFilePathString =
            Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath();
        try
        {
            // save config to disk after getPackagesToInstall, which generates package thumbprints for m_config
            SulDownloader::sdds3Wrapper()->saveConfig(m_config, configFilePathString);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR(ex.what());
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Failed to store SDDS3 config file, error:" << ex.what());
        }

        std::vector<sophlib::sdds3::PackageRef> allPackages =
            SulDownloader::sdds3Wrapper()->getPackages(*m_session.get(), m_repo, m_config);

        m_selectedSubscriptions.clear();

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

            // Don't install features we don't want
            if (!doesFeatureCodeMatchConfig(productMetadata.getFeatures()))
            {
                LOGDEBUG("Product does not match any requested features, not installing: " << product.getLine());
                continue;
            }

            // Install products that have changed
            for (auto& packageToInstall : packagesToInstall)
            {
                if (packageToInstall.lineId_ == package.lineId_)
                {
                    LOGDEBUG("Product requires install due to new package or supplement: " << product.getLine());
                    product.setProductHasChanged(true);
                    m_willInstall = true;
                    break;
                }
            }

            // Install products that fulfill missing feature codes, but that haven't changed
            if (!product.productHasChanged() && Common::UpdateUtilities::shouldProductBeInstalledBasedOnFeatures(
                                                    productMetadata.getFeatures(), installedFeatures, m_configFeatures))
            {
                LOGDEBUG("Already downloaded product requires install: " << product.getLine());
                product.setProductHasChanged(true);
                m_willInstall = true;
            }

            product.setDistributePath(Common::FileSystem::join(
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
        std::vector<sophlib::sdds3::Suite> suites =
            SulDownloader::sdds3Wrapper()->getSuites(*m_session.get(), m_repo, m_config);
        for (auto& suite : suites)
        {
            m_selectedSubscriptions.push_back({ suite.name_, suite.marketing_version_, {} });
        }
    }

    void SDDS3Repository::setWillInstall(const bool willInstall)
    {
        m_willInstall = willInstall;
    }

    void SDDS3Repository::distribute()
    {
        if (hasError() || !m_willInstall)
        {
            return;
        }
        try
        {
            SulDownloader::sdds3Wrapper()->extractPackagesTo(
                *m_session.get(),
                m_repo,
                m_config,
                Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository());
        }
        catch (...)
        {
            m_error.status = RepositoryStatus::DOWNLOADFAILED;
            m_error.Description = "Failed to extract packages";
        }
    }

} // namespace SulDownloader
