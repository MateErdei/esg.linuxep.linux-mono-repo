// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <string>
namespace UpdateSchedulerImpl
{
    /**
     * LOGERRORs the explanatory string of an exception. If the exception is nested,
     * recurses to print the explanatory of the exception it holds
     * @param e
     * @param level
     */
    void log_exception(const std::exception& e, int level=0);

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


        inline static const std::string FORCE_UPDATE_ENABLED_FLAG = "sdds3.force-update";
        inline static const std::string FORCE_PAUSED_UPDATE_ENABLED_FLAG = "sdds3.force-paused-update";

    private:
        static std::string getValueFromMCSConfig(const std::string& key);
    };
}

