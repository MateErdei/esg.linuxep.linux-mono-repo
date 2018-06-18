/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConnectionSelector.h"



namespace SulDownloader
{

    std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const ConfigurationData &configurationData)
    {
        std::vector<ConnectionSetup> candidates;


        std::vector<Proxy> proxies = configurationData.proxiesList();

        for( auto & proxy: proxies)
        {
            for(auto url : configurationData.getLocalUpdateCacheUrls())
            {
                candidates.emplace_back(url, configurationData.getCredentials(), true, proxy );
            }
        }


        for( auto & proxy: proxies)
        {
            for(auto url : configurationData.getSophosUpdateUrls())
            {
                candidates.emplace_back(url, configurationData.getCredentials(), false, proxy);
            }

        }

        // TODO: sort to improve chances of using the best candidates first. LINUXEP-6117

        return candidates;
    }
//
//    void ConnectionSelector::setLastSuccessfullConnection(const ConnectionSetup & /*connectionSetup*/,
//                                                          const ConfigurationData & /*configurationData*/)
//    {
//
//    }
}