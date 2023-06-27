// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ConfigurationData.h"

#include "Logger.h"
#include "SulDownloaderException.h"

#include "ConfigurationSettings.pb.h"

#include "Common/Policy/ProductSubscription.h"

#include <Common/ProxyUtils/ProxyUtils.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <google/protobuf/util/json_util.h>

#include <iostream>

using namespace Common::Policy;

namespace
{
    bool hasEnvironmentProxy()
    {
        return (secure_getenv("https_proxy") != nullptr || secure_getenv("HTTPS_PROXY") != nullptr);
    }

    using namespace SulDownloader::suldownloaderdata;

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

ConfigurationData::ConfigurationData(const Common::Policy::UpdateSettings&)
    : m_state(State::Initialized),
      m_logLevel(LogLevel::NORMAL),
      m_forceReinstallAllProducts(false)
{
}

const Common::Policy::Credentials& ConfigurationData::getCredentials() const
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

const std::string& ConfigurationData::getJWToken() const
{
    return m_jwToken;
}

void ConfigurationData::setJWToken(const std::string& token)
{
    m_jwToken = token;
}

const std::string& ConfigurationData::getVersigPath() const
{
    return m_versigPath;
}

void ConfigurationData::setVersigPath(const std::string& path)
{
    m_versigPath = path;
}

const std::string& ConfigurationData::getUpdateCacheCertPath() const
{
    return m_updateCacheCertPath;
}

void ConfigurationData::setUpdateCacheCertPath(const std::string& path)
{
    m_updateCacheCertPath = path;
}

const std::string& ConfigurationData::getTenantId() const
{
    return m_tenantId;
}
void ConfigurationData::setTenantId(const std::string& tenantId)
{
    m_tenantId = tenantId;
}

const std::string& ConfigurationData::getDeviceId() const
{
    return m_deviceId;
}

void ConfigurationData::setDeviceId(const std::string& deviceId)
{
    m_deviceId = deviceId;
}

std::string ConfigurationData::getLocalWarehouseRepository() const
{
    return Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseRepository();
}

std::string ConfigurationData::getLocalDistributionRepository() const
{
    return Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository();
}

void ConfigurationData::setDoForcedUpdate(bool doForcedUpdate)
{
    m_doForcedUpdate = doForcedUpdate;
}

bool ConfigurationData::getDoForcedUpdate() const
{
    return m_doForcedUpdate;
}

void ConfigurationData::setDoForcedPausedUpdate(bool doForcedPausedUpdate)
{
    m_doForcedPausedUpdate = doForcedPausedUpdate;
}

bool ConfigurationData::getDoPausedForcedUpdate() const
{
    return m_doForcedPausedUpdate;
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

    std::string localWarehouseStore = Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseStoreDir();
    if (!fileSystem->isDirectory(localWarehouseStore))
    {
        LOGERROR(
            "Invalid Settings: Local warehouse directory does not exist or is not a directory: "
            << localWarehouseStore);
        return false;
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

        std::stringstream errorMsg;
        errorMsg << "Failed to process json message with error: " << status.ToString();
        throw SulDownloaderException(errorMsg.str());
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

    ProductSubscription primary = getSubscription(settings.primarysubscription());
    std::vector<ProductSubscription> products;
    for (const auto& ProtoSubscription : settings.products())
    {
        products.emplace_back(getSubscription(ProtoSubscription));
    }
    std::vector<std::string> features;
    for (const auto& feature : settings.features())
    {
        features.emplace_back(feature);
    }
    configurationData.setPrimarySubscription(primary);
    configurationData.setProductsSubscription(products);
    configurationData.setFeatures(features);
    configurationData.setJWToken(settings.jwtoken());
    configurationData.setVersigPath(settings.versigpath());
    configurationData.setUpdateCacheCertPath(settings.updatecachecertpath());
    configurationData.setTenantId(settings.tenantid());
    configurationData.setDeviceId(settings.deviceid());
    configurationData.setDoForcedUpdate(settings.forceupdate());
    configurationData.setDoForcedPausedUpdate(settings.forcepausedupdate());

    std::vector<std::string> installArgs(
        std::begin(settings.installarguments()), std::end(settings.installarguments()));

    configurationData.setInstallArguments(installArgs);

    LogLevel level = (settings.loglevel() == ::SulDownloaderProto::ConfigurationSettings_LogLevelOption_NORMAL)
                         ? LogLevel::NORMAL
                         : LogLevel::VERBOSE;
    configurationData.setLogLevel(level);

    configurationData.setForceReinstallAllProducts(settings.forcereinstallallproducts());

    std::vector<std::string> manifestnames(std::begin(settings.manifestnames()), std::end(settings.manifestnames()));

    configurationData.setManifestNames(manifestnames);

    std::vector<std::string> optionalManifestnames(std::begin(settings.optionalmanifestnames()), std::end(settings.optionalmanifestnames()));

    configurationData.setOptionalManifestNames(optionalManifestnames);

    configurationData.setUseSlowSupplements(settings.useslowsupplements());

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
    // 1. Current environment proxy
    // 2. Policy Proxy
    // 3. Saved environment proxy (saved on install)
    // 4. No Proxy
    std::vector<Proxy> options;

    // current_proxy file (whichever proxy MCS is using) added here for SDDS3 SUS Requests.
    auto currentProxy = currentMcsProxy();
    if (currentProxy.has_value())
    {
        LOGDEBUG("Proxy found in current_proxy file: " << currentProxy.value().getUrl());
        options.emplace_back(currentProxy.value());
    }

    // Policy proxy
    if (!m_policyProxy.empty())
    {
        LOGDEBUG("Proxy found in ALC Policy: " << m_policyProxy.getUrl());
        options.emplace_back(m_policyProxy);
    }

    // Environment proxy
    if (hasEnvironmentProxy())
    {
        LOGDEBUG("Proxy found in environment");
        options.emplace_back("environment:");
    }

    // Install time env proxy
    std::string savedProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getSavedEnvironmentProxyFilePath();
    if (Common::FileSystem::fileSystem()->isFile(savedProxyFilePath))
    {
        std::vector<std::string> proxyFileContent = Common::FileSystem::fileSystem()->readLines(savedProxyFilePath);
        if (!proxyFileContent.empty())
        {
            std::string savedProxyURL = proxyFileContent[0];
            Common::UtilityImpl::StringUtils::replaceAll(savedProxyURL," ","");

            auto savedProxyOpt = proxyFromSavedProxyUrl(savedProxyURL);
            if (savedProxyOpt.has_value())
            {
                LOGDEBUG("Proxy found in saved proxy file");
                options.emplace_back(savedProxyOpt.value());
            }
        }
    }

    // Direct
    options.emplace_back(NoProxy);

    return options;
}

std::string ConfigurationData::toJsonSettings(const ConfigurationData& configurationData)
{
    using namespace google::protobuf::util;
    using SulDownloaderProto::ConfigurationSettings;

    ConfigurationSettings settings;
    for (const auto& url : configurationData.getSophosUpdateUrls())
    {
        settings.add_sophosurls()->assign(url);
    }

    for (const auto& cacheUrl : configurationData.getLocalUpdateCacheUrls())
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

    settings.mutable_jwtoken()->assign(configurationData.getJWToken());
    settings.mutable_versigpath()->assign(configurationData.getVersigPath());
    settings.mutable_updatecachecertpath()->assign(configurationData.getUpdateCacheCertPath());
    settings.mutable_tenantid()->assign(configurationData.getTenantId());
    settings.mutable_deviceid()->assign(configurationData.getDeviceId());
    settings.set_forceupdate(configurationData.getDoForcedUpdate());
    settings.set_forcepausedupdate(configurationData.getDoPausedForcedUpdate());
    const auto& primarySubscription = configurationData.getPrimarySubscription();
    setProtobufEntries(primarySubscription, settings.mutable_primarysubscription());
    for (const auto& product : configurationData.getProductsSubscription())
    {
        setProtobufEntries(product, settings.add_products());
    }
    for (const auto& feature : configurationData.getFeatures())
    {
        settings.add_features(feature);
    }

    for (const auto& installarg : configurationData.getInstallArguments())
    {
        settings.add_installarguments()->assign(installarg);
    }

    if (configurationData.getLogLevel() == LogLevel::NORMAL)
    {
        settings.set_loglevel(::SulDownloaderProto::ConfigurationSettings_LogLevelOption_NORMAL);
    }
    else
    {
        settings.set_loglevel(::SulDownloaderProto::ConfigurationSettings_LogLevelOption_VERBOSE);
    }

    for (const auto& manifestName : configurationData.getManifestNames())
    {
        settings.add_manifestnames(manifestName);
    }

    settings.set_useslowsupplements(configurationData.getUseSlowSupplements());

    for (const auto& optionalManifestName : configurationData.getOptionalManifestNames())
    {
        settings.add_optionalmanifestnames(optionalManifestName);
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
        else if (proxyDetails.size() == 2)
        {
            std::string proxyUser;
            std::string  proxyPassword;
            std::string proxyAddress = proxyDetails[1];
            //extract user:password from http(s)://user:password
            std::vector<std::string> proxyCredentials = Common::UtilityImpl::StringUtils::splitString(proxyDetails[0], "//");
            if(proxyCredentials.size() == 2)
            {
                std::vector<std::string> user_password = Common::UtilityImpl::StringUtils::splitString(proxyCredentials[1], ":");
                proxyAddress = proxyCredentials[0] + "//"+ proxyAddress;
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

void ConfigurationData::setUseSlowSupplements(bool useSlowSupplements)
{
    m_useSlowSupplements = useSlowSupplements;
}

bool ConfigurationData::getUseSlowSupplements() const
{
    return m_useSlowSupplements;
}

std::optional<Proxy> ConfigurationData::currentMcsProxy()
{
    // current_proxy file (whichever proxy MCS is using) added here for SDDS3 SUS Requests.
    try
    {
        auto [address, creds] = Common::ProxyUtils::getCurrentProxy();
        if (!address.empty())
        {
            LOGDEBUG("Proxy address from " <<  Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName() << " file: " << address);
            if (!creds.empty())
            {
                LOGDEBUG("Proxy in " <<  Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName() << " has credentials");
                try
                {
                    auto [username, password] =  Common::ProxyUtils::deobfuscateProxyCreds(creds);
                    LOGDEBUG("Deobfuscated credentials from " << Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName());
                    return Proxy(address, ProxyCredentials(username, password));
                }
                catch (const std::exception& ex)
                {
                    LOGERROR("Failed to deobfuscate proxy credentials: " << ex.what());
                }
            }
            else
            {
                LOGDEBUG("Proxy in " << Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName() << " does not have credentials");
                return Proxy(address);
            }
        }
    }
    catch(const std::exception& exception)
    {
        LOGERROR("Exception throw while processing " << Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFileName() << " file");
    }

    return std::nullopt;
}
