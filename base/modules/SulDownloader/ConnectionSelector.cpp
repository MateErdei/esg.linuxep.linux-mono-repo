//
// Created by pair on 06/06/18.
//

#include "ConnectionSelector.h"
namespace SulDownloader
{

    std::vector<ConnectionSetup> ConnectionSelector::getConnectionCandidates(const ConfigurationData &configurationData)
    {

        // TODO: get proxies ( environment variable, and others)
        // TODO: get sohposurl from sophos_alias.txt
        std::vector<ConnectionSetup> candidates;

        ConnectionSetup wrongoded("http://ostia.eng.sophos/latest/Virt-vShield",
                                  Credentials( "administrator", "passwor")
        );

        ConnectionSetup hardCoded("http://ostia.eng.sophos/latest/Virt-vShield",
          Credentials( "administrator", "password")
        );
        candidates.push_back(wrongoded);
        candidates.push_back(hardCoded);
        return candidates;
    }

    void ConnectionSelector::setLastSuccessfullConnection(const ConnectionSetup & /*connectionSetup*/,
                                                          const ConfigurationData & /*configurationData*/)
    {

    }
}