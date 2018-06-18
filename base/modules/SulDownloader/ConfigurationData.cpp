/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <iostream>
#include <sys/param.h>
#include <unistd.h>
#include <cassert>
#include "ConfigurationData.h"
#include "SulDownloaderException.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Logger.h"

#include <google/protobuf/util/json_util.h>
#include <ConfigurationSettings.pb.h>
#include <sys/stat.h>

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

namespace SulDownloader
{

    ConfigurationData::ConfigurationData(const std::vector<std::string> &sophosLocationURL, Credentials credentials,
                                         const std::vector<std::string> &updateCache, Proxy proxy)
    : m_credentials(credentials)
    , m_localUpdateCacheUrls(updateCache)
    , m_proxy(proxy)
    , m_state(State::Initialized)
    {
        setSophosUpdateUrls(sophosLocationURL);
    }

    const Credentials &ConfigurationData::getCredentials() const
    {
        return m_credentials;
    }

    void ConfigurationData::setCredentials(const Credentials &credentials)
    {
        m_credentials = credentials;
    }

    const std::vector<std::string> &ConfigurationData::getSophosUpdateUrls() const
    {
        return m_sophosUpdateUrls;
    }

    void ConfigurationData::setSophosUpdateUrls(const std::vector<std::string> &sophosUpdateUrls)
    {
        m_sophosUpdateUrls = sophosUpdateUrls;

        if ( m_sophosUpdateUrls.empty())
        {
            throw SulDownloaderException("Sophos Location list cannot be empty");
        }
    }

    const std::vector<std::string> &ConfigurationData::getLocalUpdateCacheUrls() const
    {
        return m_localUpdateCacheUrls;
    }

    void ConfigurationData::setLocalUpdateCacheUrls(const std::vector<std::string> &localUpdateCacheUrls)
    {
        m_localUpdateCacheUrls = localUpdateCacheUrls;
    }

    const Proxy &ConfigurationData::getProxy() const
    {
        return m_proxy;
    }

    void ConfigurationData::setProxy(const Proxy &proxy)
    {
        m_proxy = proxy;
    }

    std::string ConfigurationData::getCertificatePath() const
    {
        return  m_certificatePath;
    }

    std::string ConfigurationData::getUpdateCacheSslCertificatePath() const
    {
        return m_updateCacheSslCertificatePath;
    }

    std::string ConfigurationData::getSystemSslCertificatePath() const
    {
        return m_systemSslCertificatePath;
    }

    void ConfigurationData::setCertificatePath(const std::string &  certificatePath)
    {
        m_certificatePath = certificatePath;
    }

    void ConfigurationData::setUpdateCacheSslCertificatePath(const std::string &  certificatePath)
    {
        m_updateCacheSslCertificatePath = certificatePath;
    }

    void ConfigurationData::setSystemSslCertificatePath(const std::string &  certificatePath)
    {
        m_systemSslCertificatePath = certificatePath;
    }

    void ConfigurationData::setInstallationRootPath(const std::string & installationRootPath)
    {
        m_installationRootPath = installationRootPath;
    }

    std::string ConfigurationData::getInstallationRootPath() const
    {
        return m_installationRootPath;
    }

    std::string ConfigurationData::getLocalWarehouseRepository() const
    {
        using namespace Common::FileSystem;
        auto fileSystem = createFileSystem();

        return fileSystem->join(getInstallationRootPath(), "update/cache/PrimaryWarehouse");
    }

    std::string ConfigurationData::getLocalDistributionRepository() const
    {
        using namespace Common::FileSystem;
        auto fileSystem = createFileSystem();

        return fileSystem->join(getInstallationRootPath(), "update/cache/Primary");
    }

    bool ConfigurationData::verifySettingsAreValid()
    {
        using namespace Common::FileSystem;

        m_state = State::FailedVerified;

        // Must have, primary product, warehouse credentials, update location

        if(m_sophosUpdateUrls.size() == 0)
        {
            LOGERROR( "Invalid Settings: No warehouse urls provided.");
            return false;
        }

        if(m_productSelection.size() == 0)
        {
            LOGERROR( "Invalid Settings: No product selection.");
            return false;
        }

        // productselection should already be ordered with primary being the first one.
        if(m_productSelection[0].Name.empty() || m_productSelection[0].Prefix == true || m_productSelection[0].Primary == false)
        {
            LOGERROR( "Invalid Settings: No primary product provided.");
            return false;
        }

        auto fileSystem = createFileSystem();

        // localRepository should either exist or be created
        std::string localWarehouseRepository = getLocalWarehouseRepository();
        if (!fileSystem->isDirectory(localWarehouseRepository))
        {
            // TODO Check if using correct mode.
            // FIXME: remove creation of directory in the verifySettingsAreValid.
            if(mkdir(localWarehouseRepository.c_str(), 0700) != 0)
            {
                LOGERROR( "Invalid Settings: Local repository path can not be created: " << localWarehouseRepository);
                return false;
            }
        }

        // certificate path should exist and contain the root.crt and ps_rootca.crt
        std::string certificatePath = getCertificatePath();
        if ( !fileSystem->exists(certificatePath) ||
             !fileSystem->exists(fileSystem->join(certificatePath, "rootca.crt")) ||
             !fileSystem->exists(fileSystem->join(certificatePath, "ps_rootca.crt"))
           )
        {
            LOGSUPPORT( "Certificate Path: " << certificatePath );
            LOGERROR( "Invalid Settings: Certificate path does not contain required files.");
            return false;
        }
        //TODO there are more settings to verify, for example, sophosurl must be valid kind of urls.

        m_state = State::Verified;
        return true;


    }

    bool ConfigurationData::isVerified() const
    {
        return m_state == State::Verified;
    }

    void ConfigurationData::addProductSelection(const ProductGUID &productGUID)
    {
        m_productSelection.push_back(productGUID);
    }

    const std::vector<ProductGUID> ConfigurationData::getProductSelection() const
    {
        return m_productSelection;
    }

    ConfigurationData::LogLevel ConfigurationData::getLogLevel() const
    {
        return LogLevel::VERBOSE;
    }


    // remember in the end to call configurationdata.verify();
    ConfigurationData ConfigurationData::fromJsonSettings( const std::string & settingsString )
    {
        using namespace google::protobuf::util;
        using SulDownloaderProto::ConfigurationSettings;

        ConfigurationSettings settings;
        auto status = JsonStringToMessage(settingsString, &settings );
        if ( !status.ok())
        {
            std::cout << "Failed to process jason message";
            throw SulDownloaderException( "Failed to process json message");
        }

        // load input string (json) into the configuration data
        // run runSULDownloader
        // and serialize teh DownloadReport into json and give the error code/or success
        auto sUrls = settings.sophosurls();
        std::vector<std::string> sophosURLs(std::begin(sUrls), std::end(sUrls) );
        sUrls = settings.updatecache();
        std::vector<std::string> updateCaches(std::begin(sUrls), std::end(sUrls) );
        Credentials credential( settings.credential().username(), settings.credential().password());
        Proxy proxy( settings.proxy().url(), Credentials(settings.proxy().credential().username(), settings.proxy().credential().password()));

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

        for(auto product : settings.fullnames())
        {
            ProductGUID productGUID;
            productGUID.Name = product;
            productGUID.releaseTag = settings.releasetag();
            productGUID.baseVersion = settings.baseversion();
            configurationData.addProductSelection(productGUID);
        }

        for(auto product : settings.prefixnames())
        {
            ProductGUID productGUID;
            productGUID.Name = product;
            productGUID.Prefix = true;
            productGUID.releaseTag = settings.releasetag();
            productGUID.baseVersion = settings.baseversion();
            configurationData.addProductSelection(productGUID);
        }

        std::vector<std::string> installArgs;
        for(auto & arg : settings.installarguments())
        {
            installArgs.push_back(arg);
        }

        configurationData.setInstallArguments(installArgs);

        configurationData.setCertificatePath(settings.certificatepath());
        configurationData.setSystemSslCertificatePath(settings.systemsslpath());
        configurationData.setUpdateCacheSslCertificatePath(settings.cacheupdatesslpath());
        return configurationData;
    }

    std::vector<std::string> ConfigurationData::getInstallArguments() const
    {
        return m_installArguments;
    }

    void ConfigurationData::setInstallArguments(const std::vector<std::string> &installArguments)
    {
        m_installArguments = installArguments;
    }

    std::vector<Proxy> ConfigurationData::proxiesList() const
    {
        std::vector<Proxy> options;
        if ( m_proxy.empty())
        {
            if ( hasEnvironmentProxy())
            {
                options.push_back( Proxy("environment:"));
            }
            options.push_back(m_proxy);
        }
        else
        {
            options.push_back(m_proxy);
        }
        return options;
    }

}