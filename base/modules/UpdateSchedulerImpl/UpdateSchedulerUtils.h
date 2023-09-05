// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"
#include "Common/Policy/UpdateSettings.h"
#include "common/StateMachineData.h"

#include <optional>
#include <string>

namespace UpdateSchedulerImpl
{
    /**
     * LOGERRORs the explanatory string of an exception. If the exception is nested,
     * recurses to print the explanatory of the exception it holds
     * @param e
     * @param level
     */
    void log_exception(const std::exception& e, int level = 0);
    void log_exception(const Common::Exceptions::IException& e, int level = 0);

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

        static std::optional<Common::Policy::UpdateSettings> getCurrentConfigurationData();
        static std::optional<Common::Policy::UpdateSettings> getPreviousConfigurationData();
        static std::pair<Common::Policy::UpdateSettings, bool> getUpdateConfigWithLatestJWT();
        static std::optional<Common::Policy::UpdateSettings> getConfigurationDataFromJsonFile(
            const std::string& filePath);

        inline static const std::string FORCE_UPDATE_ENABLED_FLAG = "sdds3.force-update";
        inline static const std::string FORCE_PAUSED_UPDATE_ENABLED_FLAG = "sdds3.force-paused-update";
        inline static const std::string SDDS3_DELTA_V2_ENABLED_FLAG = "sdds3.delta-versioning.enabled";

    private:
        static std::string getValueFromMCSConfig(const std::string& key);
    };
} // namespace UpdateSchedulerImpl
