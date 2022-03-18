/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>

#include <string>
namespace UpdateSchedulerImpl
{
    class UpdateSchedulerUtils
    {
    public:
            static std::string calculateHealth(StateData::StateMachineData stateMachineData);
            static void cleanUpMarkerFile();
            static std::string getJWToken();
            static std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> getUpdateConfigWithLatestJWT();
    };
}

