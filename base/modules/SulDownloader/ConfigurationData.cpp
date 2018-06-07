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
    bool dirExist( const std::string & path);
    bool fileExist( const std::string & path );
    bool createDir( const std::string & path );// ::mkdir(localRepository.c_str(), 0700);//FIXME
    std::string joinPath( const std::string & parentPath, const std::string & childPath);
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
        char path[MAXPATHLEN];
        char *res = ::getcwd(path, MAXPATHLEN);
        assert( res != 0);
        return std::string(res) + "/tmp/warehouse";
    }

    std::string ConfigurationData::getCertificatePath() const
    {
        return  "/home/pair/dev_certificates";
    }

    bool ConfigurationData::verifySettingsAreValid()
    {
        m_state = State::FailedVerified;
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
}