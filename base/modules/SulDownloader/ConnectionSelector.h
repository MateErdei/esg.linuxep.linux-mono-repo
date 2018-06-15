/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_CONNECTIONSELECTOR_H
#define EVEREST_CONNECTIONSELECTOR_H

#include "ConfigurationData.h"
#include "ConnectionSetup.h"
namespace SulDownloader
{
    class ConnectionSelector
    {
    public:
        std::vector<ConnectionSetup> getConnectionCandidates( const ConfigurationData & configurationData);
        void setLastSuccessfullConnection( const ConnectionSetup & connectionSetup, const ConfigurationData & configurationData);
    };

}


#endif //EVEREST_CONNECTIONSELECTOR_H
