/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConnectionSelector.h"
namespace SulDownloader
{

    std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const ConfigurationData &configurationData)
    {
        // from the list of sophos locations
        // list of update caches
        // proxies
        // create and sort by preference the candidates for the Connection Setup


        // TODO: get proxies ( environment variable, and others)
        // TODO: get sohposurl from sophos_alias.txt
        std::vector<ConnectionSetup> candidates;


        for(auto url : configurationData.getLocalUpdateCacheUrls())
        {
            candidates.emplace_back(url, configurationData.getCredentials(), true, configurationData.getProxy()); //with https
        }


        for(auto url : configurationData.getLocalUpdateCacheUrls())
        {
            candidates.emplace_back(url, configurationData.getCredentials(), true, configurationData.getProxy(), false); //without
        }

        for(auto url : configurationData.getSophosUpdateUrls())
        {
            candidates.emplace_back(url, configurationData.getCredentials(), false, configurationData.getProxy());
        }

        // TODO: sort to improve chances of using the best candidates first. LINUXEP-6117

        return candidates;
    }

    void ConnectionSelector::setLastSuccessfullConnection(const ConnectionSetup & /*connectionSetup*/,
                                                          const ConfigurationData & /*configurationData*/)
    {

    }
}