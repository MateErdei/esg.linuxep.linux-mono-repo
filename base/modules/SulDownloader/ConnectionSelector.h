/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_CONNECTIONSELECTOR_H
#define EVEREST_CONNECTIONSELECTOR_H

#include "ConfigurationData.h"
#include "ConnectionSetup.h"
namespace SulDownloader
{
    /**
     * Given the list of sophos urls, update caches and proxies, there are a variety of possible ways to connect to the warehouse.
     *
     * ConnectionSelector is responsible to create all the valid ways to connect to the Warehouse while also ordering them
     * in a way to maximize the possibility of connection success.
     */
    class ConnectionSelector
    {
    public:
        std::vector<ConnectionSetup> getConnectionCandidates( const ConfigurationData & configurationData);
        void setLastSuccessfullConnection( const ConnectionSetup & connectionSetup, const ConfigurationData & configurationData);
    };

}


#endif //EVEREST_CONNECTIONSELECTOR_H
