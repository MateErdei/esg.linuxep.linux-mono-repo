// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ConfigurationData.h"

#include "Logger.h"
#include "SulDownloaderException.h"

#include "Common/Policy/ProductSubscription.h"
#include "Common/Policy/SerialiseUpdateSettings.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#include <Common/ProxyUtils/ProxyUtils.h>
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
/*
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
    }*/
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
    Proxy policyProxy)
{
    forceReinstallAllProducts_ = false;
    credentials_ = credentials;
    localUpdateCacheHosts_ = updateCache;
    setSophosUpdateUrls(sophosLocationURL);
    setPolicyProxy(policyProxy);
}

ConfigurationData::ConfigurationData(const Common::Policy::UpdateSettings& settings)
{
    forceReinstallAllProducts_ = settings.getForceReinstallAllProducts();
    credentials_ = settings.getCredentials();
    setSophosUpdateUrls(settings.getSophosLocationURLs());
    setLocalUpdateCacheHosts(settings.getLocalUpdateCacheHosts());
    setPrimarySubscription(settings.getPrimarySubscription());
    setProductsSubscription(settings.getProductsSubscription());
    setUseSlowSupplements(settings.getUseSlowSupplements());
    setFeatures(settings.getFeatures());
    setPolicyProxy(settings.getPolicyProxy());
}

const Common::Policy::Credentials& ConfigurationData::getCredentials() const
{
    return credentials_;
}

const std::vector<std::string>& ConfigurationData::getSophosUpdateUrls() const
{
    return sophosLocationURLs_;
}

void ConfigurationData::setSophosUpdateUrls(const std::vector<std::string>& sophosUpdateUrls)
{
    if (sophosUpdateUrls.empty())
    {
        sophosLocationURLs_ = DefaultSophosLocationsURL;
    }
    else
    {
        sophosLocationURLs_ = sophosUpdateUrls;
    }

    if (sophosLocationURLs_.empty())
    {
        throw SulDownloaderException("Sophos Location list cannot be empty");
    }
}

const std::vector<std::string>& ConfigurationData::getLocalUpdateCacheUrls() const
{
    return localUpdateCacheHosts_;
}

const Proxy& ConfigurationData::getPolicyProxy() const
{
    return policyProxy_;
}

void ConfigurationData::setPolicyProxy(const Proxy& proxy)
{
    policyProxy_ = proxy;
}


void ConfigurationData::setForceReinstallAllProducts(const bool forceReinstallAllProducts)
{
    forceReinstallAllProducts_ = forceReinstallAllProducts;
}

bool ConfigurationData::getForceReinstallAllProducts() const
{
    return forceReinstallAllProducts_;
}

UpdateSettings ConfigurationData::fromJsonSettings(const std::string& settingsString)
{
    return Common::Policy::SerialiseUpdateSettings::fromJsonSettings(settingsString);
}

const std::vector<std::string>& ConfigurationData::getInstallArguments() const
{
    return installArguments_;
}

const std::vector<std::string>& ConfigurationData::getManifestNames() const
{
    return manifestNames_;
}

void ConfigurationData::setManifestNames(const std::vector<std::string>& manifestNames)
{
    manifestNames_ = manifestNames;
}

const std::vector<std::string>& ConfigurationData::getOptionalManifestNames() const
{
    return optionalManifestNames_;
}

void ConfigurationData::setOptionalManifestNames(const std::vector<std::string>& optionalManifestNames)
{
    optionalManifestNames_ = optionalManifestNames;
}

std::vector<Proxy> ConfigurationData::proxiesList(const UpdateSettings& settings)
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

    auto policyProxy = settings.getPolicyProxy();
    // Policy proxy
    if (!policyProxy.empty())
    {
        LOGDEBUG("Proxy found in ALC Policy: " << policyProxy.getUrl());
        options.emplace_back(policyProxy);
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

std::vector<Proxy> ConfigurationData::proxiesList() const
{
    return proxiesList(*this);
}

std::string ConfigurationData::toJsonSettings(const UpdateSettings& updateSettings)
{
    return Common::Policy::SerialiseUpdateSettings::toJsonSettings(updateSettings);
}

const ProductSubscription& ConfigurationData::getPrimarySubscription() const
{
    return primarySubscription_;
}

const std::vector<ProductSubscription>& ConfigurationData::getProductsSubscription() const
{
    return productSubscriptions_;
}

const std::vector<std::string>& ConfigurationData::getFeatures() const
{
    return features_;
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
    useSlowSupplements_ = useSlowSupplements;
}

bool ConfigurationData::getUseSlowSupplements() const
{
    return useSlowSupplements_;
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
