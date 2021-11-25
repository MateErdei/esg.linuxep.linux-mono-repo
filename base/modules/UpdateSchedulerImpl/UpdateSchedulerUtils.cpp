/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerUtils.h"


#include <json.hpp>
namespace UpdateSchedulerImpl
{

    std::string UpdateSchedulerUtils::calculateHealth(StateData::StateMachineData stateMachineData)
    {

        auto const& downloadState = stateMachineData.getDownloadState();
        auto const& installState = stateMachineData.getInstallState();

        nlohmann::json healthResponseMessage;
        healthResponseMessage["downloadState"] = 0;
        healthResponseMessage["installState"] = 0;
        healthResponseMessage["overall"] = 0;
        if (downloadState != "0")
        {
            healthResponseMessage["downloadState"] = 1;
            healthResponseMessage["overall"] = 1;
        }
        if (installState != "0")
        {
            healthResponseMessage["installState"] = 1;
            healthResponseMessage["overall"] = 1;
        }

        return healthResponseMessage.dump();
    }
}