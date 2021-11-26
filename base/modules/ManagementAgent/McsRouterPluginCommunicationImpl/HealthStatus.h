/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <ManagementAgent/PluginCommunication/PluginHealthStatus.h>

#include <string>
#include <map>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class HealthStatus
        {
        public:
            HealthStatus();
            ~HealthStatus() = default;
            /**
             * Function to clear the internal containers 'i.e. maps' to ensure the generated xml only contains
             * the current set of plugin information.
             */
            void resetPluginHealthLists();

            /**
             * Add plugin health status object to the health status information used to generate the health status xml
             * @param pluginName
             * @param status the PluginHealthStatus object containing the health status and type of health.
             */
            void addPluginHealth(const std::string& pluginName, const PluginCommunication::PluginHealthStatus& status);

            /**
             * generates a xml string to be used for the health status sent to central.
             * @return pair<bool, string> true if status has changes false otherwise, and the status xml string
             */
            std::pair<bool, std::string> generateHealthStatusXml();

            /**
             * Clear cached XML status to force update of status file
             */
            void resetCachedHealthStatusXml();

        private:
            unsigned int convertDetailedValueToOverallValue(unsigned int value);
            void updateOverallHealthStatus();
            void updateServiceHealthXml(
                const std::string& typeName,
                std::stringstream& statusXml,
                std::map<std::string, PluginCommunication::PluginHealthStatus>& healthMap,
                unsigned int overallHealthValue);

            std::map<std::string, PluginCommunication::PluginHealthStatus> m_pluginServiceHealth;
            std::map<std::string, PluginCommunication::PluginHealthStatus> m_pluginThreatServiceHealth;
            std::map<std::string, PluginCommunication::PluginHealthStatus> m_pluginThreatDetectionHealth;
            unsigned int m_overallHealth;
            unsigned int m_overallPluginServiceHealth;
            unsigned int m_overallPluginThreatServiceHealth;
            unsigned int m_overallPluginThreatDetectionHealth;
            std::string m_cachedHealthStatusXml;
        };
    }
}

