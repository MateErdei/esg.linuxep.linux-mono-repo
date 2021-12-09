/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <string>
namespace UpdateSchedulerImpl
{
    class UpdateSchedulerUtils
    {
    public:
            static std::string calculateHealth(StateData::StateMachineData stateMachineData);
            static void cleanUpMarkerFile();
    };
}

