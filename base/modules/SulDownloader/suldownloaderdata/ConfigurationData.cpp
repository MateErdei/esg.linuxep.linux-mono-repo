/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigurationData.h"

#include "Logger.h"
#include "SulDownloaderException.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#include <google/protobuf/util/json_util.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <ConfigurationSettings.pb.h>
#include <iostream>

namespace
{
    using namespace SulDownloader::suldownloaderdata;

    bool hasEnvironmentProxy()
    {
        return (secure_getenv("https_proxy") != nullptr || secure_getenv("HTTPS_PROXY") != nullptr);
    }

    void setProtobufEntries(
        const ProductSubscription& subscription,
        SulDownloaderProto::ConfigurationSettings_Subscription* proto_subscription)
    {
        assert( proto_subscription != nullptr);
        proto_subscription->set_rigidname(subscription.rigidName());
        proto_subscription->set_baseversion(subscription.baseVersion());
        proto_subscription->set_tag(subscription.tag());
        proto_subscription->set_fixedversion(subscription.fixedVersion());
    }

    ProductSubscription getSubscription(
        const SulDownloaderProto::ConfigurationSettings_Subscription& proto_subscription)
    {
        return ProductSubscription(
            proto_subscription.rigidname(),
            proto_subscription.baseversion(),
            proto_subscription.tag(),
            proto_subscription.fixedversion());
    }

    bool isProductSubscriptionValid(const ProductSubscription& productSubscription)
    {
        if (productSubscription.rigidName().empty())
        {
            LOGWARN("Invalid Settings: Empty RigidName.");
            return false;
        }
        if (productSubscription.fixedVersion().empty() && productSubscription.tag().empty())
        {
            LOGWARN("Invalid Settings: Product can not have both FixedVersion and Tag empty.");
            return false;
        }
        return true;
    }

} // namespace

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

const std::string ConfigurationData::DoNotSetSslSystemPath(":system:");
const std::vector<std::string> ConfigurationData::DefaultSophosLocationsURL{ "http://dci.sophosupd.com/update",
                                                                             "http://dci.sophosupd.net/update" };

ConfigurationData::ConfigurationData(
    const std::vector<std::string>& sophosLocationURL,
    Credentials credentials,
    const std::vector<std::string>& updateCache,
    Proxy policyProxy) :
    m_credentials(std::move(credentials)),
    m_localUpdateCacheUrls(updateCache),
    m_policyProxy(std::move(policyProxy)),
    m_state(State::Initialized),
    m_logLevel(LogLevel::NORMAL),
    m_forceReinstallAllProducts(false)
{
    setSophosUpdateUrls(sophosLocationURL);
}

const Credentials& ConfigurationData::getCredentials() const
{
    return m_credentials;
}

void ConfigurationData::setCredentials(const Credentials& credentials)
{
    m_credentials = credentials;
}

const std::vector<std::string>& ConfigurationData::getSophosUpdateUrls() const
{
    return m_sophosUpdateUrls;
}

void ConfigurationData::setSophosUpdateUrls(const std::vector<std::string>& sophosUpdateUrls)
{
    if (sophosUpdateUrls.empty())
    {
        m_sophosUpdateUrls = DefaultSophosLocationsURL;
    }
    else
    {
        m_sophosUpdateUrls = sophosUpdateUrls;
    }

    if (m_sophosUpdateUrls.empty())
    {
        throw SulDownloaderException("Sophos Location list cannot be empty");
    }
}

const std::vector<std::string>& ConfigurationData::getLocalUpdateCacheUrls() const
{
    return m_localUpdateCacheUrls;
}

void ConfigurationData::setLocalUpdateCacheUrls(const std::vector<std::string>& localUpdateCacheUrls)
{
    m_localUpdateCacheUrls = localUpdateCacheUrls;
}

const Proxy& ConfigurationData::getPolicyProxy() const
{
    return m_policyProxy;
}

void ConfigurationData::setPolicyProxy(const Proxy& proxy)
{
    m_policyProxy = proxy;
}

std::string ConfigurationData::getCertificatePath() const
{
    if (m_certificatePath.empty())
    {
        return Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath();
    }

    return m_certificatePath;
}

const std::string& ConfigurationData::getUpdateCacheSslCertificatePath() const
{
    return m_updateCacheSslCertificatePath;
}

const std::string& ConfigurationData::getSystemSslCertificatePath() const
{
    return m_systemSslCertificatePath;
}

void ConfigurationData::setCertificatePath(const std::string& certificatePath)
{
    m_certificatePath = certificatePath;
}

void ConfigurationData::setUpdateCacheSslCertificatePath(const std::string& certificatePath)
{
    m_updateCacheSslCertificatePath = certificatePath;
}

void ConfigurationData::setSystemSslCertificatePath(const std::string& certificatePath)
{
    if (certificatePath == "")
    {
        m_systemSslCertificatePath = DoNotSetSslSystemPath;
    }
    else
    {
        m_systemSslCertificatePath = certificatePath;
    }
}

void ConfigurationData::setInstallationRootPath(const std::string& installationRootPath)
{
    if (installationRootPath.empty())
    {
        return;
    }
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, installationRootPath);
}

std::string ConfigurationData::getInstallationRootPath() const
{
    return Common::ApplicationConfiguration::applicationConfiguration().getData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL);
}

std::string ConfigurationData::getLocalWarehouseRepository() const
{
    return Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseRepository();
}

std::string ConfigurationData::getLocalDistributionRepository() const
{
    return Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository();
}

bool ConfigurationData::verifySettingsAreValid()
{
    using namespace Common::FileSystem;

    m_state = State::FailedVerified;

    // Must have, primary product, warehouse credentials, update location

    if (m_sophosUpdateUrls.empty())
    {
        LOGERROR("Invalid Settings: No sophos update urls provided.");
        return false;
    }
    else
    {
        for (auto& value : m_sophosUpdateUrls)
        {
            if (value.empty())
            {
                LOGERROR("Invalid Settings: Sophos update url provided cannot be an empty string.");
                return false;
            }
        }
    }

    if (m_credentials.getUsername().empty())
    {
        LOGERROR("Invalid Settings: Credential 'username' cannot be empty string.");
        return false;
    }

    if (m_credentials.getPassword().empty())
    {
        LOGERROR("Invalid Settings: Credential 'password' cannot be empty string.");
        return false;
    }

    if (!isProductSubscriptionValid(getPrimarySubscription()))
    {
        LOGERROR("Invalid Settings: No primary product provided.");
        return false;
    }

    for (auto& productSubscription : getProductsSubscription())
    {
        if (!isProductSubscriptionValid(productSubscription))
        {
            return false;
        }
    }

    auto features = getFeatures();
    if (features.empty())
    {
        LOGERROR("Empty feature set");
        return false;
    }

    if (std::find(features.begin(), features.end(), "CORE") == features.end())
    {
        LOGERROR("CORE feature not in the feature set. ");
        return false;
    }

    auto fileSystem = Common::FileSystem::fileSystem();

    std::string installationRootPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    if (!fileSystem->isDirectory(installationRootPath))
    {
        LOGERROR(
            "Invalid Settings: installation root path does not exist or is not a directory: " << installationRootPath);
        return false;
    }

    std::string localWarehouseRepository = getLocalWarehouseRepository();
    if (!fileSystem->isDirectory(localWarehouseRepository))
    {
        LOGERROR(
            "Invalid Settings: Local warehouse repository does not exist or is not a directory: "
            << localWarehouseRepository);
        return false;
    }

    std::string localDistributionRepository = getLocalDistributionRepository();
    if (!fileSystem->isDirectory(localDistributionRepository))
    {
        LOGERROR(
            "Invalid Settings: Local distribution repository does not exist or is not a directory: "
            << localDistributionRepository);
        return false;
    }

    // certificate path should exist and contain the root.crt and ps_rootca.crt
    std::string certificatePath = getCertificatePath();
    if (!fileSystem->exists(certificatePath) ||
        !fileSystem->exists(Common::FileSystem::join(certificatePath, "rootca.crt")) ||
        !fileSystem->exists(Common::FileSystem::join(certificatePath, "ps_rootca.crt")))
    {
        LOGSUPPORT("Certificate Path: " << certificatePath);
        LOGERROR("Invalid Settings: Certificate path does not contain required files.");
        return false;
    }

    std::string systemSslCertificatePath = getSystemSslCertificatePath();

    if (systemSslCertificatePath != DoNotSetSslSystemPath && (!fileSystem->exists(systemSslCertificatePath)))
    {
        LOGERROR("Invalid Settings: system ssl certificate path does not exist: " << systemSslCertificatePath);
        return false;
    }

    std::string updateCacheSslCertificatePath = getUpdateCacheSslCertificatePath();

    if (updateCacheSslCertificatePath.empty() && !getLocalUpdateCacheUrls().empty())
    {
        LOGERROR("Invalid Settings: Update cache ssl certificate path cannot be empty when using update caches.");
        return false;
    }
    else if (!updateCacheSslCertificatePath.empty() && !getLocalUpdateCacheUrls().empty())
    {
        if (!fileSystem->exists(updateCacheSslCertificatePath))
        {
            LOGERROR(
                "Invalid Settings: Local distribution repository does not exist : " << updateCacheSslCertificatePath);
            return false;
        }
    }

    if (!m_localUpdateCacheUrls.empty())
    {
        for (auto& value : m_localUpdateCacheUrls)
        {
            if (value.empty())
            {
                LOGERROR("Invalid Settings: Update cache url cannot be an empty string");
                return false;
            }
        }
    }

    if (!m_installArguments.empty())
    {
        for (auto& value : m_installArguments)
        {
            if (value.empty())
            {
                LOGERROR("Invalid Settings: install argument cannot be an empty string");
                return false;
            }
        }
    }

    m_state = State::Verified;
    return true;
}

bool ConfigurationData::isVerified() const
{
    return m_state == State::Verified;
}

ConfigurationData::LogLevel ConfigurationData::getLogLevel() const
{
    return m_logLevel;
}

void ConfigurationData::setLogLevel(ConfigurationData::LogLevel level)
{
    m_logLevel = level;
}

void ConfigurationData::setForceReinstallAllProducts(const bool forceReinstallAllProducts)
{
    m_forceReinstallAllProducts = forceReinstallAllProducts;
}

bool ConfigurationData::getForceReinstallAllProducts() const
{
    return m_forceReinstallAllProducts;
}

ConfigurationData ConfigurationData::fromJsonSettings(const std::string& settingsString)
{
    using namespace google::protobuf::util;
    using SulDownloaderProto::ConfigurationSettings;

    ConfigurationSettings settings;
    JsonParseOptions jsonParseOptions;
    jsonParseOptions.ignore_unknown_fields = true; 

    auto status = JsonStringToMessage(settingsString, &settings, jsonParseOptions);
    if (!status.ok())
    {
        LOGERROR("Failed to process input settings");
        LOGSUPPORT(status.ToString());
        throw SulDownloaderException("Failed to process json message");
    }

    // load input string (json) into the configuration data
    // run runSULDownloader
    // and serialize the DownloadReport into json and give the error code/or success
    auto sUrls = settings.sophosurls();
    std::vector<std::string> sophosURLs(std::begin(sUrls), std::end(sUrls));

    sUrls = settings.updatecache();
    std::vector<std::string> updateCaches(std::begin(sUrls), std::end(sUrls));

    Credentials credential(settings.credential().username(), settings.credential().password());
    Proxy proxy(
        settings.proxy().url(),
        ProxyCredentials(
            settings.proxy().credential().username(),
            settings.proxy().credential().password(),
            settings.proxy().credential().proxytype()));

    ConfigurationData configurationData(sophosURLs, credential, updateCaches, proxy);

    configurationData.setCertificatePath(settings.certificatepath());
    configurationData.setInstallationRootPath(settings.installationrootpath());

    ProductSubscription primary = getSubscription(settings.primarysubscription());
    std::vector<ProductSubscription> products;
    for (auto& ProtoSubscription : settings.products())
    {
        products.emplace_back(getSubscription(ProtoSubscription));
    }
    std::vector<std::string> features;
    for (auto& feature : settings.features())
    {
        features.emplace_back(feature);
    }
    configurationData.setPrimarySubscription(primary);
    configurationData.setProductsSubscription(products);
    configurationData.setFeatures(features);

    std::vector<std::string> installArgs(
        std::begin(settings.installarguments()), std::end(settings.installarguments()));

    configurationData.setInstallArguments(installArgs);

    configurationData.setCertificatePath(settings.certificatepath());
    configurationData.setSystemSslCertificatePath(settings.systemsslpath());
    configurationData.setUpdateCacheSslCertificatePath(settings.cacheupdatesslpath());
    LogLevel level = (settings.loglevel() == ::SulDownloaderProto::ConfigurationSettings_LogLevelOption_NORMAL)
                         ? LogLevel::NORMAL
                         : LogLevel::VERBOSE;
    configurationData.setLogLevel(level);

    configurationData.setForceReinstallAllProducts(settings.forcereinstallallproducts());

    std::vector<std::string> manifestnames(std::begin(settings.manifestnames()), std::end(settings.manifestnames()));

    configurationData.setManifestNames(manifestnames);

    std::vector<std::string> optionalManifestnames(std::begin(settings.optionalmanifestnames()), std::end(settings.optionalmanifestnames()));

    configurationData.setOptionalManifestNames(optionalManifestnames);

    return configurationData;
}

const std::vector<std::string>& ConfigurationData::getInstallArguments() const
{
    return m_installArguments;
}

void ConfigurationData::setInstallArguments(const std::vector<std::string>& installArguments)
{
    m_installArguments = installArguments;
}

const std::vector<std::string>& ConfigurationData::getManifestNames() const
{
    return m_manifestNames;
}

void ConfigurationData::setManifestNames(const std::vector<std::string>& manifestNames)
{
    m_manifestNames = manifestNames;
}

const std::vector<std::string>& ConfigurationData::getOptionalManifestNames() const
{
    return m_optionalManifestNames;
}

void ConfigurationData::setOptionalManifestNames(const std::vector<std::string>& optionalManifestNames)
{
    m_optionalManifestNames = optionalManifestNames;
}


std::vector<Proxy> ConfigurationData::proxiesList() const
{
    // This generates the list of proxies in order that they should be tried by SUL
    // 1. Policy Proxy
    // 2. Current environment proxy
    // 3. Saved environment proxy (saved on install)
    // 4. No Proxy
    std::vector<Proxy> options;
    if (!m_policyProxy.empty())
    {
        options.emplace_back(m_policyProxy);
    }
    if (hasEnvironmentProxy())
    {
        options.emplace_back("environment:");
    }
    std::string savedProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getSavedEnvironmentProxyFilePath();
    if (Common::FileSystem::fileSystem()->isFile(savedProxyFilePath))
    {
        std::string savedProxyURL = Common::FileSystem::fileSystem()->readFile(savedProxyFilePath);

        auto savedProxyOpt = proxyFromSavedProxyUrl(savedProxyURL);
        if(savedProxyOpt.has_value())
        {
            options.emplace_back(savedProxyOpt.value());
        }
    }
    options.emplace_back(Proxy(Proxy::NoProxy));

    return options;
}

std::string ConfigurationData::toJsonSettings(const ConfigurationData& configurationData)
{
    using namespace google::protobuf::util;
    using SulDownloaderProto::ConfigurationSettings;

    ConfigurationSettings settings;
    for (auto& url : configurationData.getSophosUpdateUrls())
    {
        settings.add_sophosurls()->assign(url);
    }

    for (auto& cacheUrl : configurationData.getLocalUpdateCacheUrls())
    {
        settings.add_updatecache()->assign(cacheUrl);
    }

    settings.mutable_credential()->set_username(configurationData.getCredentials().getUsername());
    settings.mutable_credential()->set_password(configurationData.getCredentials().getPassword());

    settings.mutable_proxy()->mutable_credential()->set_username(
        configurationData.getPolicyProxy().getCredentials().getUsername());
    settings.mutable_proxy()->mutable_credential()->set_password(
        configurationData.getPolicyProxy().getCredentials().getPassword());
    settings.mutable_proxy()->mutable_credential()->set_proxytype(
        configurationData.getPolicyProxy().getCredentials().getProxyType());
    settings.mutable_proxy()->mutable_url()->assign(configurationData.getPolicyProxy().getUrl());

    settings.set_certificatepath(configurationData.getCertificatePath());
    settings.set_installationrootpath(configurationData.getInstallationRootPath());

    const auto& primarySubscription = configurationData.getPrimarySubscription();
    setProtobufEntries(primarySubscription, settings.mutable_primarysubscription());
    for (auto& product : configurationData.getProductsSubscription())
    {
        setProtobufEntries(product, settings.add_products());
    }
    for (auto& feature : configurationData.getFeatures())
    {
        settings.add_features(feature);
    }

    for (auto& installarg : configurationData.getInstallArguments())
    {
        settings.add_installarguments()->assign(installarg);
    }

    settings.set_certificatepath(configurationData.getCertificatePath());

    settings.set_systemsslpath(configurationData.getSystemSslCertificatePath());
    settings.set_cacheupdatesslpath(configurationData.getUpdateCacheSslCertificatePath());

    if (configurationData.getLogLevel() == LogLevel::NORMAL)
    {
        settings.set_loglevel(::SulDownloaderProto::ConfigurationSettings_LogLevelOption_NORMAL);
    }
    else
    {
        settings.set_loglevel(::SulDownloaderProto::ConfigurationSettings_LogLevelOption_VERBOSE);
    }

    for (auto& manifestName : configurationData.getManifestNames())
    {
        settings.add_manifestnames(manifestName);
    }

    return Common::ProtobufUtil::MessageUtility::protoBuf2Json(settings);
}

void ConfigurationData::setPrimarySubscription(const ProductSubscription& productSubscription)
{
    m_primarySubscription = productSubscription;
}

void ConfigurationData::setProductsSubscription(const std::vector<ProductSubscription>& productsSubscriptions)
{
    m_productsSubscription = productsSubscriptions;
}

const ProductSubscription& ConfigurationData::getPrimarySubscription() const
{
    return m_primarySubscription;
}

const std::vector<ProductSubscription>& ConfigurationData::getProductsSubscription() const
{
    return m_productsSubscription;
}

void ConfigurationData::setFeatures(const std::vector<std::string>& features)
{
    m_features = features;
}

const std::vector<std::string>& ConfigurationData::getFeatures() const
{
    return m_features;
}

const std::string ProductSubscription::toString() const
{
    std::stringstream s;
    s << "name = " << m_rigidName << " baseversion = " << m_baseVersion << " tag = " << m_tag
      << " fixedversion = " << m_fixedVersion;
    return s.str();
}

std::optional<Proxy> ConfigurationData::proxyFromSavedProxyUrl(const std::string& savedProxyURL)
{
    //savedProxyURL: can be in the following formats
    // http(s)://username:password@192.168.0.1:8080
    // http(s)://192.168.0.1
    // http(s)://192.168.0.1:8080

    if( savedProxyURL.find("https://") != std::string::npos || savedProxyURL.find("http://") != std::string::npos)
    {
        std::vector<std::string> proxyDetails = Common::UtilityImpl::StringUtils::splitString(savedProxyURL, "@");
        if (proxyDetails.size() == 1)
        {
            return Proxy(savedProxyURL);
        }
        else if(proxyDetails.size() == 2)
        {
            std::string proxyUser;
            std::string  proxyPassword;
            std::string proxyAddress = proxyDetails[1];
            //extract user:password from http(s)://user:password
            std::vector<std::string> proxyCredentials = Common::UtilityImpl::StringUtils::splitString(proxyDetails[0], "//");
            if(proxyCredentials.size() == 2)
            {
                std::vector<std::string> user_password = Common::UtilityImpl::StringUtils::splitString(proxyCredentials[1], ":");
                if (user_password.size() == 2)
                {
                    proxyUser = user_password[0];
                    proxyPassword = user_password[1];
                    return Proxy(proxyAddress, ProxyCredentials(proxyUser, proxyPassword, {}));
                }
            }
        }
    }
    // not logging proxy here in-case it contains username and passwords.
    LOGWARN("Proxy URL not in expected format.");
    return std::nullopt;
}