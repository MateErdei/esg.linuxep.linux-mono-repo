/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConnectionSelector.h"

#include "Logger.h"

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const ConfigurationData& configurationData)
{
    std::vector<ConnectionSetup> candidates;

    std::vector<Proxy> proxies = configurationData.proxiesList();

    // Requirement: With update cache no proxy url must be given but the credentials are still necessary.
    // if the proxy is set then, then we only pass the credentials data for the proxy to the update cache proxy
    // settings. If no credentials are required for proxy then empty strings are passed  - this is ok.
    Proxy proxyForUpdateCache("noproxy:", proxies[0].getCredentials());

    for (const auto& url : configurationData.getLocalUpdateCacheUrls())
    {
        candidates.emplace_back(url, configurationData.getCredentials(), true, proxyForUpdateCache);
        LOGDEBUG("Adding Update Cache connection candidate, URL: " << url << ", proxy: " << proxyForUpdateCache.getUrl());
    }

    for (auto& proxy : proxies)
    {
        for (const auto& url : configurationData.getSophosUpdateUrls())
        {
            candidates.emplace_back(url, configurationData.getCredentials(), false, proxy);
            LOGDEBUG("Adding connection candidate, URL: " << url << ", proxy: " << proxy.getUrl());
        }
    }

    return candidates;
}

std::vector<ConnectionSetup> ConnectionSelector::getSDDS3ConnectionCandidates(const ConfigurationData& configurationData)
{
    std::vector<ConnectionSetup> candidates;

    std::vector<Proxy> proxies = configurationData.proxiesList();

    // Requirement: With update cache no proxy url must be given but the credentials are still necessary.
    // if the proxy is set then, then we only pass the credentials data for the proxy to the update cache proxy
    // settings. If no credentials are required for proxy then empty strings are passed  - this is ok.
    Proxy proxyForUpdateCache("noproxy:", proxies[0].getCredentials());

    for (const auto& url : configurationData.getLocalUpdateCacheUrls())
    {
        candidates.emplace_back(url, configurationData.getCredentials(), true, proxyForUpdateCache);
    }

    for (auto& proxy : proxies)
    {
        if (proxy.getUrl() != Proxy::NoProxy)
        {
            candidates.emplace_back("", configurationData.getCredentials(), false, proxy);
        }

    }

    return candidates;
}