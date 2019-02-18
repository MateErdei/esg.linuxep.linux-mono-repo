/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConnectionSetup.h"

#include "SulDownloaderException.h"

#include <cassert>
#include <sstream>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

ConnectionSetup::ConnectionSetup(
    const std::string& updateLocationURL,
    const Credentials& credentials,
    bool isCacheUpdate,
    const Proxy& proxy) :
    m_credentials(credentials),
    m_isUpdateCache(isCacheUpdate),
    m_proxy(proxy)
{
    setUpdateLocationURL(updateLocationURL);
}

const Credentials& ConnectionSetup::getCredentials() const
{
    return m_credentials;
}

void ConnectionSetup::setCredentials(const Credentials& credentials)
{
    m_credentials = credentials;
}

const Proxy& ConnectionSetup::getProxy() const
{
    return m_proxy;
}

void ConnectionSetup::setProxy(const Proxy& proxy)
{
    m_proxy = proxy;
}

void ConnectionSetup::setUpdateLocationURL(const std::string& updateLocationURL)
{
    m_updateLocationURL = updateLocationURL;
    assert(!m_updateLocationURL.empty());
}

std::string ConnectionSetup::toString() const
{
    std::stringstream ss;
    if (isCacheUpdate())
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

const std::string& ConnectionSetup::getUpdateLocationURL() const
{
    return m_updateLocationURL;
}
