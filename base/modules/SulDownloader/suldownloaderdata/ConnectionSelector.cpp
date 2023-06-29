// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ConnectionSelector.h"

#include "ConfigurationData.h"
#include "Logger.h"

#include "Common/Policy/Proxy.h"

using namespace Common::Policy;
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const UpdateSettings& updateSettings)
{
    std::vector<ConnectionSetup> candidates;

    std::vector<Proxy> proxies = SulDownloader::suldownloaderdata::ConfigurationData::proxiesList(updateSettings);

    // Requirement: With update cache no proxy url must be given but the credentials are still necessary.
    // if the proxy is set then, then we only pass the credentials data for the proxy to the update cache proxy
    // settings. If no credentials are required for proxy then empty strings are passed  - this is ok.
    Proxy proxyForUpdateCache("noproxy:", proxies[0].getCredentials());

    for (const auto& url : updateSettings.getLocalUpdateCacheHosts())
    {
        candidates.emplace_back(url, true, proxyForUpdateCache);
        LOGDEBUG("Adding Update Cache connection candidate, URL: " << url << ", proxy: " << proxyForUpdateCache.getUrl());
    }

    for (auto& proxy : proxies)
    {
        for (const auto& url : updateSettings.getSophosLocationURLs())
        {
            candidates.emplace_back(url, false, proxy);
            LOGDEBUG("Adding connection candidate, URL: " << url << ", proxy: " << proxy.getUrl());
        }
    }

    return candidates;
}

std::vector<ConnectionSetup> ConnectionSelector::getSDDS3ConnectionCandidates(const UpdateSettings& updateSettings)
{
    std::vector<ConnectionSetup> candidates;

    std::vector<Proxy> proxies = SulDownloader::suldownloaderdata::ConfigurationData::proxiesList(updateSettings);

    // Requirement: With update cache no proxy url must be given but the credentials are still necessary.
    // if the proxy is set then, then we only pass the credentials data for the proxy to the update cache proxy
    // settings. If no credentials are required for proxy then empty strings are passed  - this is ok.
    Proxy proxyForUpdateCache("noproxy:", proxies[0].getCredentials());

    for (const auto& url : updateSettings.getLocalUpdateCacheHosts())
    {
        candidates.emplace_back(url, true, proxyForUpdateCache);
    }

    for (auto& proxy : proxies)
    {
        if (proxy.getUrl() != NoProxy)
        {
            candidates.emplace_back("", false, proxy);
        }
    }

    return candidates;
}