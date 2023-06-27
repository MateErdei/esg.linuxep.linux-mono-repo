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