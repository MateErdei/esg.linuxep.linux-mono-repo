/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        enum class HealthType
        {
            NONE,
            SERVICE,
            THREAT_SERVICE,
            SERVICE_AND_THREAT,
            THREAT_DETECTION
        };

        struct PluginHealthStatus
        {
            PluginHealthStatus() : healthType(HealthType::NONE), healthValue(0)
            {
            }

            HealthType healthType;
            std::string displayName;
            unsigned int healthValue;
        };

//        static void to_json( nlohmann::json& j, const PluginHealthStatus& phs)
//        {
//            j =  nlohmann::json{
//                {"healthType", phs.healthType},
//                {"displayName", phs.displayName},
//                {"healthValue", phs.healthValue}};
//        }
//
//        static void from_json(const  nlohmann::json& j, PluginHealthStatus& phs)
//        {
//            j.at("healthType").get_to(phs.healthType);
//            j.at("displayName").get_to(phs.displayName);
//            j.at("healthValue").get_to(phs.healthValue);
//        }
    }
}

