// Copyright 2021-2024 Sophos Limited. All rights reserved.

#pragma once

#include "ManagementAgent/HealthStatusCommon/PluginHealthStatus.h"

#include <map>
#include <string>

namespace ManagementAgent::HealthStatusImpl
{
    class HealthStatus
    {
    public:
        using healthValue_t = PluginCommunication::PluginHealthStatus::healthValue_t;
        HealthStatus();
        ~HealthStatus();
        struct HealthInfo
        {
            bool hasStatusChanged;
            std::string statusXML;
            bool hasOverallHealthChanged;
            std::string healthJson;
        };
        /**
         * Function to clear the internal containers 'i.e. maps' to ensure the generated xml only contains
         * the current set of plugin information.
         */
        void resetPluginHealthLists();

        /**
         * Function to set threat value back to good (1). This can only be called by a manual reset request
         * from Central or when a full AV Scan returns healthy.
         */
        void resetThreatDetectionHealth();

        /**
         * Function to get the plugin service health map
         */
        std::map<std::string, PluginCommunication::PluginHealthStatus> getPluginServiceHealthLists();

        /**
         * Function to get the plugin threat service health map
         */
        std::map<std::string, PluginCommunication::PluginHealthStatus> getPluginThreatServiceHealthLists();

        /**
         * Add plugin health status object to the health status information used to generate the health status xml
         * @param pluginName
         * @param status the PluginHealthStatus object containing the health status and type of health.
         */
        void addPluginHealth(const std::string& pluginName, const PluginCommunication::PluginHealthStatus& status);

        /**
         * generates a xml string to be used for the health status sent to central.
         * generates a JSON of the health status
         * @return HealthInfo with true if status has changes false otherwise, and the status xml string
         */
        HealthInfo generateHealthStatusXml();

        /**
         * Clear cached XML status to force update of status file
         */
        void resetCachedHealthStatusXml();

        void setOutbreakMode(bool outbreakStatus);

    private:
        static healthValue_t convertDetailedValueToOverallValue(healthValue_t value);
        void updateOverallHealthStatus();
        static void updateServiceHealthXml(
            const std::string& typeName,
            std::stringstream& statusXml,
            std::map<std::string, PluginCommunication::PluginHealthStatus>& healthMap,
            healthValue_t overallHealthValue);
        void storeThreatHealth();
        void loadThreatHealth();

        std::map<std::string, PluginCommunication::PluginHealthStatus> m_pluginServiceHealth;
        std::map<std::string, PluginCommunication::PluginHealthStatus> m_pluginThreatServiceHealth;
        std::map<std::string, PluginCommunication::PluginHealthStatus> m_pluginThreatDetectionHealth;
        healthValue_t m_overallHealth = 0;
        healthValue_t m_overallPluginServiceHealth = 0;
        healthValue_t m_overallPluginThreatServiceHealth = 0;
        healthValue_t m_overallPluginThreatDetectionHealth = 0;
        std::string m_activeHeartbeatUtmId;
        std::string m_cachedHealthStatusXml;
        bool m_activeHeartbeat = false;
        bool outbreakStatus_ = false;
        bool isolated_ = false;
    };
} // namespace ManagementAgent::HealthStatusImpl