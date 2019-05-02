/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Proxy.h"

using namespace SulDownloader::suldownloaderdata;

const std::string Proxy::NoProxy("noproxy:"); // nolint

Proxy::Proxy(std::string url, SulDownloader::suldownloaderdata::ProxyCredentials credentials) :
    m_url(std::move(url)),
    m_credentials(std::move(credentials))
{
}

const ProxyCredentials& Proxy::getCredentials() const
{
    return m_credentials;
}

const std::string& Proxy::getUrl() const
{
    return m_url;
}

bool Proxy::empty() const
{
    return m_url.empty() || m_url == NoProxy;
}

std::string Proxy::toStringPostfix() const
{
    if (getUrl() == "noproxy:")
    {
        return " directly";
    }
    else if (getUrl() == "environment:")
    {
        return " via environment proxy";
    }
    else if (getUrl().empty())
    {
        return " via environment proxy or directly";
    }
    else
    {
        return std::string("via proxy: ") + getUrl();
    }
}
