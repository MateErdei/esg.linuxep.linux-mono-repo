//
// Created by pair on 06/06/18.
//

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
