//
// Created by pair on 05/06/18.
//

#include <sys/param.h>
#include <unistd.h>
#include <cassert>
#include "ConfigurationData.h"
#include "SulDownloaderException.h"



namespace
{
//    bool dirExist( const std::string & path);
//    bool fileExist( const std::string & path );
//    bool createDir( const std::string & path );// ::mkdir(localRepository.c_str(), 0700);//FIXME
//    std::string joinPath( const std::string & parentPath, const std::string & childPath);
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

    std::string ConfigurationData::getLocalRepository() const
    {

        return m_localRepository;

    }

    std::string ConfigurationData::getCertificatePath() const
    {
        return  m_certificatePath;
    }

    void ConfigurationData::setCertificatePath(const std::string &  certificatePath)
    {
        m_certificatePath = certificatePath;
    }

    void ConfigurationData::setLocalRepository(const std::string & localRepository)
    {
        m_localRepository = localRepository;
    }

    bool ConfigurationData::verifySettingsAreValid()
    {
        m_state = State::FailedVerified;

        // Must have, primary product, warehouse credentials, update location

        if(m_sophosUpdateUrls.size() == 0)
        {
            return false;
        }

        if(m_productSelection.size() == 0)
        {
            return false;
        }

        // productselection should already be ordered with primary being the first one.
        if(m_productSelection[0].Name.empty() || m_productSelection[0].Prefix == true || m_productSelection[0].Primary == false)
        {
            return false;
        }

//
//        // localRepository should either exist or be created
//        std::string localRepository = getLocalRepository();
//        if ( !dirExist(localRepository) && !createDir(localRepository))
//        {
//            return false;
//
//        }
//
//        // certificate path should exist and contain the root.crt and ps_rootca.crt
//        std::string certificatePath = getCertificatePath();
//        if ( !dirExist(certificatePath) ||
//             !fileExist( joinPath(certificatePath, "root.crt")) ||
//             !fileExist( joinPath(certificatePath, "ps_rootca.crt"))
//           )
//        {
//            return false;
//        }


        m_state = State::Verified;
        return false;


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

    std::string ConfigurationData::getSSLCertificatePath() const {
        return  "/etc/ssl/certs";
    }

    ConfigurationData::LogLevel ConfigurationData::getLogLevel() const {
        return LogLevel::VERBOSE;
    }
}