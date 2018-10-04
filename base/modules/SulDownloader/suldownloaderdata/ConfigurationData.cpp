/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "ConfigurationData.h"
#include "SulDownloaderException.h"

#include <Common/FileSystem/IFileSystem.h>
#include <ConfigurationSettings.pb.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/MessageUtility.h>

#include <google/protobuf/util/json_util.h>
#include <iostream>

namespace
{
    bool hasEnvironmentProxy()
    {
        return (secure_getenv("https_proxy") != nullptr ||
                secure_getenv("HTTPS_PROXY") != nullptr ||
                secure_getenv("http_proxy") != nullptr ||
                secure_getenv("HTTP_PROXY") != nullptr);
    }
}

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

const std::string ConfigurationData::DoNotSetSslSystemPath(":system:");
const std::vector<std::string> ConfigurationData::DefaultSophosLocationsURL{"http://dci.sophosupd.com/update"
                                                                            , "http://dci.sophosupd.net/update"};

ConfigurationData::ConfigurationData(const std::vector<std::string>& sophosLocationURL,
                                     Credentials credentials,
                                     const std::vector<std::string>& updateCache,
                                     Proxy proxy)
        : m_credentials(std::move(credentials))
          , m_localUpdateCacheUrls(updateCache)
          , m_proxy(std::move(proxy))
          , m_state(State::Initialized)
          , m_logLevel(LogLevel::NORMAL)
          , m_forceReinstallAllProducts(false)
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
    m_sophosUpdateUrls = sophosUpdateUrls;

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

const Proxy& ConfigurationData::getProxy() const
{
    return m_proxy;
}

void ConfigurationData::setProxy(const Proxy& proxy)
{
    m_proxy = proxy;
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
    m_systemSslCertificatePath = certificatePath;
}

void ConfigurationData::setInstallationRootPath(const std::string& installationRootPath)
{
    if (installationRootPath.empty())
    {
        return;
    }
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, installationRootPath
    );
}

std::string ConfigurationData::getInstallationRootPath() const
{
    return Common::ApplicationConfiguration::applicationConfiguration().getData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL
    );
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

    if (m_productSelection.empty())
    {
        LOGERROR("Invalid Settings: No product selection.");
        return false;
    }
    else
    {
        for (auto& value : m_productSelection)
        {
            if (value.releaseTag.empty())
            {
                LOGERROR("Invalid Settings: ReleaseTag cannot be an empty string");
                return false;
            }

            if (value.baseVersion.empty())
            {
                LOGERROR("Invalid Settings: Base version cannot be an empty string");
                return false;
            }

            if (value.Name.empty() && !value.Prefix)
            {
                LOGERROR("Invalid Settings: Full product name cannot be an empty string");
                return false;
            }

            if (value.Name.empty() && value.Prefix)
            {
                LOGERROR("Invalid Settings: Prefix product name cannot be an empty string");
                return false;
            }
        }
    }


    // productselection should already be ordered with primary being the first one.
    if (m_productSelection[0].Name.empty() || m_productSelection[0].Prefix || !m_productSelection[0].Primary)
    {
        LOGERROR("Invalid Settings: No primary product provided.");
        return false;
    }

    auto fileSystem = Common::FileSystem::fileSystem();

    std::string installationRootPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    if (!fileSystem->isDirectory(installationRootPath))
    {
        LOGERROR("Invalid Settings: installation root path does not exist or is not a directory: "
                         << installationRootPath);
        return false;
    }


    std::string localWarehouseRepository = getLocalWarehouseRepository();
    if (!fileSystem->isDirectory(localWarehouseRepository))
    {
        LOGERROR("Invalid Settings: Local warehouse repository does not exist or is not a directory: "
                         << localWarehouseRepository);
        return false;
    }

    std::string localDistributionRepository = getLocalDistributionRepository();
    if (!fileSystem->isDirectory(localDistributionRepository))
    {
        LOGERROR("Invalid Settings: Local distribution repository does not exist or is not a directory: "
                         << localDistributionRepository);
        return false;
    }


    // certificate path should exist and contain the root.crt and ps_rootca.crt
    std::string certificatePath = getCertificatePath();
    if (!fileSystem->exists(certificatePath) ||
        !fileSystem->exists(Common::FileSystem::join(certificatePath, "rootca.crt")) ||
        !fileSystem->exists(Common::FileSystem::join(certificatePath, "ps_rootca.crt"))
            )
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
            LOGERROR("Invalid Settings: Local distribution repository does not exist : "
                             << updateCacheSslCertificatePath);
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

void ConfigurationData::addProductSelection(const ProductGUID& productGUID)
{
    m_productSelection.push_back(productGUID);
}

const std::vector<ProductGUID>& ConfigurationData::getProductSelection() const
{
    return m_productSelection;
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
    auto status = JsonStringToMessage(settingsString, &settings);
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
    Proxy proxy(settings.proxy().url(),
                ProxyCredentials(settings.proxy().credential().username(),
                                 settings.proxy().credential().password(),
                                 settings.proxy().credential().proxytype()));

    ConfigurationData configurationData(sophosURLs, credential, updateCaches, proxy);

    configurationData.setCertificatePath(settings.certificatepath());
    configurationData.setInstallationRootPath(settings.installationrootpath());


    ProductGUID primaryProductGUID;
    primaryProductGUID.Name = settings.primary();
    primaryProductGUID.Primary = true;
    primaryProductGUID.Prefix = false;
    primaryProductGUID.releaseTag = settings.releasetag();
    primaryProductGUID.baseVersion = settings.baseversion();
    configurationData.addProductSelection(primaryProductGUID);

    for (auto& product : settings.fullnames())
    {
        ProductGUID productGUID;
        productGUID.Name = product;
        productGUID.Primary = false;
        productGUID.Prefix = false;
        productGUID.releaseTag = settings.releasetag();
        productGUID.baseVersion = settings.baseversion();
        configurationData.addProductSelection(productGUID);
    }

    for (auto& product : settings.prefixnames())
    {
        ProductGUID productGUID;
        productGUID.Name = product;
        productGUID.Primary = false;
        productGUID.Prefix = true;
        productGUID.releaseTag = settings.releasetag();
        productGUID.baseVersion = settings.baseversion();
        configurationData.addProductSelection(productGUID);
    }

    std::vector<std::string> installArgs(std::begin(settings.installarguments()),
                                         std::end(settings.installarguments()));

    configurationData.setInstallArguments(installArgs);

    configurationData.setCertificatePath(settings.certificatepath());
    configurationData.setSystemSslCertificatePath(settings.systemsslpath());
    configurationData.setUpdateCacheSslCertificatePath(settings.cacheupdatesslpath());
    LogLevel level = (settings.loglevel() ==
                      ::SulDownloaderProto::ConfigurationSettings_LogLevelOption_NORMAL) ? LogLevel::NORMAL
                                                                                         : LogLevel::VERBOSE;
    configurationData.setLogLevel(level);

    configurationData.setForceReinstallAllProducts(settings.forcereinstallallproducts());

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

std::vector<Proxy> ConfigurationData::proxiesList() const
{
    std::vector<Proxy> options;
    if (m_proxy.empty() && hasEnvironmentProxy())
    {
        options.emplace_back("environment:");
    }

    options.push_back(m_proxy);
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
            configurationData.getProxy().getCredentials().getUsername());
    settings.mutable_proxy()->mutable_credential()->set_password(
            configurationData.getProxy().getCredentials().getPassword());
    settings.mutable_proxy()->mutable_credential()->set_proxytype(
            configurationData.getProxy().getCredentials().getProxyType());
    settings.mutable_proxy()->mutable_url()->assign(configurationData.getProxy().getUrl());

    settings.set_certificatepath(configurationData.getCertificatePath());
    settings.set_installationrootpath(configurationData.getInstallationRootPath());

    auto productsGUID = configurationData.getProductSelection();

    if (!productsGUID.empty())
    {
        auto& primaryProduct = productsGUID[0];
        // the established convention is that the first one should be the primary
        assert(primaryProduct.Primary);
        settings.set_primary(primaryProduct.Name);
        settings.set_baseversion(primaryProduct.baseVersion);
        settings.set_releasetag(primaryProduct.releaseTag);
    }
    // skip first as it is the primary
    for (size_t i = 1; i < productsGUID.size(); i++)
    {
        auto& secondaryProduct = productsGUID[i];
        if (secondaryProduct.Prefix)
        {
            settings.add_prefixnames()->assign(secondaryProduct.Name);
        }
        else
        {
            settings.add_fullnames()->assign(secondaryProduct.Name);
        }
    }

    for (auto& installarg: configurationData.getInstallArguments())
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

    return Common::UtilityImpl::MessageUtility::protoBuf2Json(settings);

}
