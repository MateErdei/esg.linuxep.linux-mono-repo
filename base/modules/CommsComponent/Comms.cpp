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
        std::string sophosInstall = "/opt/sophos-spl";

        auto commnsProcess = [](std::shared_ptr<MessageChannel> channel, IOtherSideApi & api){
            CommsDistributor distributor{ Common::ApplicationConfiguration::applicationPathManager().getCommsRequestDirPath(), "",
                Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath(), *channel, api}; 
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

        // FIXME: warning is being shown that Failed to read the config file /opt/sophos-spl/base/etc/logger.conf. All settings will be set to their default value
        // Need to work out if it is related to the parent or child process.
        
        CommsConfigurator configurator{CommsConfigurator::chrootPathForSSPL(sophosInstall), childUser, parentUser, CommsConfigurator::getListOfDependenciesToMount()};
        return splitProcessesReactors(commnsProcess, std::move(commsNetwork), configurator ); 
    }
} // namespace CommsComponent