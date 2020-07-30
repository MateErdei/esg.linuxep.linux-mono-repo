/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#
#include <Common/UtilityImpl/ProjectNames.h>
#include "SplitProcesses.h"
#include "ReactorAdapter.h"
#include "NetworkSide.h"
#include "CommsDistributor.h"
#include "Configurator.h"

namespace CommsComponent {

    int main_entry() {        
        std::string sophosInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();


        auto commsProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi & api){
            CommsDistributor distributor{ Common::ApplicationConfiguration::applicationPathManager().getCommsRequestDirPath(), "",
                Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath(), *channel, api, true}; 
            distributor.handleRequestsAndResponses(); 
            return 0; 
        }; 

        CommsNetwork commsNetwork; 
        UserConf childUser; 
        UserConf parentUser;
        childUser.logName="comms-network"; 
        childUser.userName = sophos::networkUser(); 
        childUser.userGroup = sophos::localGroup(); 

        parentUser.logName = "comms_component"; 
        parentUser.userName = sophos::localUser(); 
        parentUser.userGroup = sophos::localGroup(); 
     
        CommsConfigurator configurator{CommsConfigurator::chrootPathForSSPL(sophosInstall), childUser, parentUser, CommsConfigurator::getListOfDependenciesToMount()};
        return splitProcessesReactors(commsProcess, std::move(commsNetwork), configurator );
    }
} // namespace CommsComponent