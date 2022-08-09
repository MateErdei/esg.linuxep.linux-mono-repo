/******************************************************************************************************

Copyright 2021-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <string>
namespace UpdateSchedulerImpl
{
    class UpdateSchedulerUtils
    {
    public:
        static std::string calculateHealth(StateData::StateMachineData stateMachineData);
        static void cleanUpMarkerFile();
        static std::string readMarkerFile();
        static std::string getJWToken();
        static std::string getTenantId();
        static std::string getDeviceId();
        static std::string getSDDSMechanism(bool sdds3Enabled);

        static std::optional<SulDownloader::suldownloaderdata::ConfigurationData> getCurrentConfigurationData();
        static std::optional<SulDownloader::suldownloaderdata::ConfigurationData> getPreviousConfigurationData();
        static std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> getUpdateConfigWithLatestJWT();
        static std::optional<SulDownloader::suldownloaderdata::ConfigurationData> getConfigurationDataFromJsonFile(const std::string& filePath);

        inline static const std::string SDDS3_ENABLED_FLAG = "sdds3.enabled";

    private:
        static std::string getValueFromMCSConfig(const std::string& key);
    };
}

