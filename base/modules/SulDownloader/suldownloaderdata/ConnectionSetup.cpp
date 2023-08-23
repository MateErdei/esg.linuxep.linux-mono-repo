// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ConnectionSetup.h"

#include <sstream>

using namespace SulDownloader::suldownloaderdata;

ConnectionSetup::ConnectionSetup(const std::string& updateLocationURL, bool isCacheUpdate, const proxy_t& proxy) :
    m_updateLocationURL(updateLocationURL), m_isUpdateCache(isCacheUpdate), m_proxy(proxy)
{
}

const ConnectionSetup::proxy_t& ConnectionSetup::getProxy() const
{
    return m_proxy;
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