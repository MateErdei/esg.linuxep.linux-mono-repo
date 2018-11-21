/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConnectionSelector.h"

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const ConfigurationData& configurationData)
{
    std::vector<ConnectionSetup> candidates;


    std::vector<Proxy> proxies = configurationData.proxiesList();

    // Requirement: With update cache no proxy url must be given but the credentials are still necessary.
    // if the proxy is set then, then we only pass the credentials data for the proxy to the update cache proxy settings.
    // If no credentials are required for proxy then empty strings are passed  - this is ok.
    Proxy proxyForUpdateCache("", proxies[0].getCredentials());

    for (auto url : configurationData.getLocalUpdateCacheUrls())
    {
        candidates.emplace_back(url, configurationData.getCredentials(), true, proxyForUpdateCache);
    }

    for (auto& proxy: proxies)
    {
        for (auto url : configurationData.getSophosUpdateUrls())
        {
            candidates.emplace_back(url, configurationData.getCredentials(), false, proxy);
        }
    }

    return candidates;
}
//
//    void ConnectionSelector::setLastSuccessfullConnection(const ConnectionSetup & /*connectionSetup*/,
//                                                          const ConfigurationData & /*configurationData*/)
//    {
//
//    }