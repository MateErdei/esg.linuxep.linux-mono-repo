/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "HealthStatus.h"

#include <sstream>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        HealthStatus::HealthStatus()
        : m_overallHealth(0)
        , m_overallPluginServiceHealth(0)
        , m_overallPluginThreatServiceHealth(0)
        , m_overallPluginThreatDetectionHealth(0)
        , m_cachedHealthStatusXml("")
        {

        }

        void HealthStatus::resetPluginHealthLists()
        {
            m_pluginServiceHealth.clear();
            m_pluginThreatServiceHealth.clear();
        }

        void HealthStatus::resetThreatDetectionHealth()
        {
            for (auto& health : m_pluginThreatDetectionHealth)
            {
                health.second.healthValue = 1;
            }
        }

        void HealthStatus::addPluginHealth(const std::string& pluginName, const PluginCommunication::PluginHealthStatus& status)
        {
            switch (status.healthType)
            {
                case PluginCommunication::HealthType::SERVICE:
                    m_pluginServiceHealth[pluginName] = status;
                    break;
                case PluginCommunication::HealthType::THREAT_SERVICE:
                    m_pluginThreatServiceHealth[pluginName] = status;
                    break;
                case PluginCommunication::HealthType::SERVICE_AND_THREAT:
                    m_pluginServiceHealth[pluginName] = status;
                    m_pluginThreatServiceHealth[pluginName] = status;
                    break;
                case PluginCommunication::HealthType::THREAT_DETECTION:
                    m_pluginThreatDetectionHealth[pluginName] = status;
                    break;
                case PluginCommunication::HealthType::NONE:
                    // should never get here.
                    break;
            }
        }

        unsigned int HealthStatus::convertDetailedValueToOverallValue(unsigned int value)
        {
            if (value == 0)
            {
                return 1;
            }
            else if (value == 1 || value == 2)
            {
                return 3;
            }

            return 3; // unexpected value report bad state.
        }

        void HealthStatus::updateOverallHealthStatus()
        {
            // done this in one method, but we can break it out if needs be.
            unsigned int healthValue = 1;
            for (auto health : m_pluginServiceHealth)
            {
                healthValue = std::max(convertDetailedValueToOverallValue(health.second.healthValue), healthValue);
            }
            m_overallPluginServiceHealth = healthValue;

            healthValue = 1;
            for (auto health : m_pluginThreatServiceHealth)
            {
                healthValue = std::max(convertDetailedValueToOverallValue(health.second.healthValue), healthValue);
            }
            m_overallPluginThreatServiceHealth = healthValue;

            healthValue = 1;
            for (auto health : m_pluginThreatDetectionHealth)
            {
                healthValue = std::max(health.second.healthValue, healthValue);
            }
            m_overallPluginThreatDetectionHealth = healthValue;

            healthValue = std::max(m_overallPluginServiceHealth, m_overallPluginThreatServiceHealth);
            healthValue = std::max(m_overallPluginThreatDetectionHealth, healthValue);
            m_overallHealth = healthValue;

        }

        void HealthStatus::updateServiceHealthXml(
            const std::string& typeName,
            std::stringstream& statusXml,
            std::map<std::string, PluginCommunication::PluginHealthStatus>& healthMap,
            unsigned int overallHealthValue)
        {
            statusXml
                << R"(<item name=")" << typeName << R"(" value=")" << overallHealthValue << R"(" >)";

            for (auto& status : healthMap)
            {
                statusXml
                    << R"(<detail name=")" << status.second.displayName <<  R"(" value=")" << status.second.healthValue << R"(" />)";
            }

            statusXml << R"(</item>)";
        }

        std::pair<bool,std::string> HealthStatus::generateHealthStatusXml()
        {
            updateOverallHealthStatus();
            std::stringstream statusXml;

            statusXml
                    << R"(<?xml version="1.0" encoding="utf-8" ?>)"
                    << R"(<health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId="">)"
                    << R"(<item name="health" value=")" << m_overallHealth << R"(" />)";

            if (!m_pluginServiceHealth.empty())
            {
               updateServiceHealthXml("service", statusXml, m_pluginServiceHealth, m_overallPluginServiceHealth );
            }

            if (!m_pluginThreatServiceHealth.empty())
            {
                updateServiceHealthXml("threatService", statusXml, m_pluginThreatServiceHealth, m_overallPluginThreatServiceHealth );
            }

            statusXml
                    << R"(<item name="threat" value=")" << m_overallPluginThreatDetectionHealth << R"(" />)"
                    << R"(</health>)";

            bool hasStatusChanged = (statusXml.str() != m_cachedHealthStatusXml);
            m_cachedHealthStatusXml = statusXml.str();
            return std::make_pair(hasStatusChanged, statusXml.str());
        }
        void HealthStatus::resetCachedHealthStatusXml()
        {
            m_cachedHealthStatusXml.clear();
        }
    }
}
