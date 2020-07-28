/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include "SplitProcesses.h"
#include "ReactorAdapter.h"
#include "NetworkSide.h"
#include "CommsDistributor.h"
#include "Configurator.h"

namespace CommsComponent {

    int main_entry() {        
        std::string sophosInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall(); 

        auto commnsProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi & api){
            CommsDistributor distributor{ Common::ApplicationConfiguration::applicationPathManager().getCommsRequestDirPath(), "",
                Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath(), *channel, api}; 
            distributor.handleRequestsAndResponses(); 
            return 0; 
        }; 

        CommsNetwork commsNetwork; 
        //CommsConfigurator configurator{CommsConfigurator::chrootPathForSSPL(sophosInstall), sophos::networkUser(), sophos::localUser(), CommsConfigurator::getListOfDependenciesToMount()};
        //return splitProcessesReactors(commnsProcess, std::move(commsNetwork), configurator ); 
        return splitProcessesReactors(commnsProcess, std::move(commsNetwork) ); 
    }
} // namespace CommsComponent