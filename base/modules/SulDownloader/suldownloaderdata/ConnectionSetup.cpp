// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ConnectionSetup.h"

#include "SulDownloaderException.h"

#include <cassert>
#include <sstream>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

ConnectionSetup::ConnectionSetup(
    const std::string& updateLocationURL,
    bool isCacheUpdate,
    const proxy_t& proxy) :
    m_isUpdateCache(isCacheUpdate),
    m_proxy(proxy)
{
    setUpdateLocationURL(updateLocationURL);
}

const ConnectionSetup::proxy_t& ConnectionSetup::getProxy() const
{
    return m_proxy;
}

void ConnectionSetup::setProxy(const proxy_t& proxy)
{
    m_proxy = proxy;
}

void ConnectionSetup::setUpdateLocationURL(const std::string& updateLocationURL)
{
    m_updateLocationURL = updateLocationURL;
}

std::string ConnectionSetup::toString() const
{
    std::stringstream ss;
    if (isCacheUpdate())
    {
        ss << "Update cache at ";
    }
    else
    {
        ss << "Sophos at ";
    }

    ss << m_updateLocationURL << " " << m_proxy.toStringPostfix();
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
