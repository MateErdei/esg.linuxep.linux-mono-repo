// Copyright 2023 Sophos Limited. All rights reserved.
#include "Proxy.h"

using namespace Common::Policy;

Proxy::Proxy(std::string url, ProxyCredentials credentials) : url_(std::move(url)), credentials_(std::move(credentials))
{
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