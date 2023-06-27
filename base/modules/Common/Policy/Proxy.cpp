// Copyright 2023 Sophos Limited. All rights reserved.
#include "Proxy.h"

#include "Common/UtilityImpl/StringUtils.h"

using namespace Common::Policy;

Proxy::Proxy(std::string url, ProxyCredentials credentials) : url_(std::move(url)), credentials_(std::move(credentials))
{}

std::string Proxy::getProxyUrlAsSulRequires() const
{
    if (url_.empty() || url_ == NoProxy || url_ == EnvironmentProxy)
    {
        return url_;
    }
    if (Common::UtilityImpl::StringUtils::startswith(url_, "http://") ||
        Common::UtilityImpl::StringUtils::startswith(url_, "https://"))
    {
        return url_;
    }
    // sul requires proxy to be pre-pended with http://
    return "http://" + url_;
}

std::string Proxy::toStringPostfix() const
{
    if (getUrl() == NoProxy)
    {
        return " directly";
    }
    else if (getUrl() == EnvironmentProxy)
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

/*

using namespace SulDownloader::suldownloaderdata;

const std::string Proxy::NoProxy("noproxy:");              // nolint
// if you change this const you will also need to change it in the sau Linuxhttp lib so that the sdds3lib continues to work
const std::string Proxy::EnvironmentProxy("environment:"); // nolint

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

std::string Proxy::getProxyUrlAsSulRequires() const
{
    if (m_url.empty() || m_url == NoProxy || m_url == EnvironmentProxy)
    {
        return m_url;
    }
    if (Common::UtilityImpl::StringUtils::startswith(m_url, "http://") ||
        Common::UtilityImpl::StringUtils::startswith(m_url, "https://"))
    {
        return m_url;
    }
    // sul requires proxy to be pre-pended with http://
    return "http://" + m_url;
}
*/