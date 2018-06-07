//
// Created by pair on 05/06/18.
//

#include <cassert>
#include "ConnectionSetup.h"
#include "SulDownloaderException.h"
namespace SulDownloader
{

    ConnectionSetup::ConnectionSetup(const std::vector<std::string> &sophosLocationURL, Credentials credentials ,
                    const std::string &updateCache, Proxy proxy)
     : m_credentials(credentials)
     , m_updateCache(updateCache)
     , m_proxy(proxy)
    {
        setSophosLocationURL(sophosLocationURL);
    }

    const Credentials &ConnectionSetup::getCredentials() const
    {
        return m_credentials;
    }

    void ConnectionSetup::setCredentials(const Credentials &credentials)
    {
        m_credentials = credentials;
    }

    const Proxy &ConnectionSetup::getProxy() const
    {
        return m_proxy;
    }

    void ConnectionSetup::setProxy(const Proxy &proxy)
    {
        m_proxy = proxy;
    }

    const std::string &ConnectionSetup::getUpdateCache() const
    {
        return m_updateCache;
    }

    void ConnectionSetup::setUpdateCache(const std::string &updateCache)
    {
        m_updateCache = updateCache;
    }

    const std::vector<std::string> &ConnectionSetup::getSophosLocationURL() const
    {
        return m_sophosLocationURL;
    }

    void ConnectionSetup::setSophosLocationURL(const std::vector<std::string> &sophosLocationURL)
    {
        m_sophosLocationURL = sophosLocationURL;
        assert( !m_sophosLocationURL.empty());
    }
}