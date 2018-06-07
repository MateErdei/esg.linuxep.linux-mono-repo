//
// Created by pair on 06/06/18.
//

#include "ConnectionSelector.h"
namespace SulDownloader
{

    std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const ConfigurationData &configurationData)
    {

        std::vector<ConnectionSetup> candidates;

        ConnectionSetup hardCoded(std::vector<std::string>{"http://ostia.eng.sophos/latest/Virt-vShield"},
          Credentials( "administrator", "password")
        );
        candidates.push_back(hardCoded);
        return candidates;
    }

    void ConnectionSelector::setLastSuccessfullConnection(const ConnectionSetup & /*connectionSetup*/,
                                                          const ConfigurationData & /*configurationData*/)
    {

    }
}