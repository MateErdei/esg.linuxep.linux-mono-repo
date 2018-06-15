/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <cassert>
#include <sstream>
#include "ConnectionSetup.h"
#include "SulDownloaderException.h"
namespace SulDownloader
{

    ConnectionSetup::ConnectionSetup(const std::string updateLocationURL, Credentials credentials, bool isCacheUpdate,
                                     Proxy proxy, bool useHTTPS)
    : m_credentials(credentials)
    , m_isUpdateCache(isCacheUpdate)
    , m_proxy(proxy)
    , m_useHTTPS(useHTTPS)
    {
        setUpdateLocationURL(updateLocationURL);
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

    void ConnectionSetup::setUpdateLocationURL(const std::string &updateLocationURL)
    {
        m_updateLocationURL = updateLocationURL;
        assert( !m_updateLocationURL.empty());
    }


    std::string ConnectionSetup::toString() const
    {
        std::stringstream ss;
        if ( isCacheUpdate())
        {
            ss << "Update cache Url: ";
        }
        else
        {
            ss << "Sophos Url: ";
        }

        ss << m_updateLocationURL << "\n";

        if (!m_proxy.empty())
        {
            ss << "Proxy url: " << m_proxy.getUrl();
        }

        return ss.str();

    }

    bool ConnectionSetup::isCacheUpdate() const
    {
        return m_isUpdateCache;
    }



    const std::string &ConnectionSetup::getUpdateLocationURL() const
    {
        return m_updateLocationURL;
    }

    bool ConnectionSetup::useHTTPS() const {
        return m_useHTTPS;
    }

}