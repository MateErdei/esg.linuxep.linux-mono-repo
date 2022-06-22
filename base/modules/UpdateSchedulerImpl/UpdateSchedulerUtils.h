/******************************************************************************************************

Copyright 2021-2022 Sophos Limited.  All rights reserved.

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
            static std::string getTenantId();
            static std::string getDeviceId();

            static std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> getUpdateConfigWithLatestJWT();
            static bool isFlagSet(const std::string& flag, const std::string& flagContent);

            inline static const std::string SDDS3_ENABLED_FLAG = "sdds3.enabled";
        private:
        static std::string getValueFromMCSConfig(const std::string& key);
    };
}

