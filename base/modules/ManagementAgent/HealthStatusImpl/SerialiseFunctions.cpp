/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SerialiseFunctions.h"

#include <ManagementAgent/PluginCommunication/PluginHealthStatus.h>

#include <json.hpp>

namespace ManagementAgent::HealthStatusImpl
{
    std::string serialiseThreatHealth(
        const std::map<std::string, PluginCommunication::PluginHealthStatus>& pluginThreatDetectionHealth)
    {
        try
        {
            // Possible compiler bug, make sure to init the json object and not rely on compiler to
            // default construct it else dump() will return "null": https://github.com/nlohmann/json/issues/2046
            nlohmann::json pluginHealthStatusJsonObj(nlohmann::json::value_t::object);
            for (const auto& [pluginName, pluginHealthStatus] : pluginThreatDetectionHealth)
            {
                pluginHealthStatusJsonObj[pluginName]["displayName"] = pluginHealthStatus.displayName;
                pluginHealthStatusJsonObj[pluginName]["healthValue"] = pluginHealthStatus.healthValue;
                pluginHealthStatusJsonObj[pluginName]["healthType"] = pluginHealthStatus.healthType;
            }
            return pluginHealthStatusJsonObj.dump();
        }
        catch (const std::exception& exception)
        {
            LOGERROR("Failed to serialise Threat Health object: " << exception.what());
        }
        return "";
    }

    std::map<std::string, PluginCommunication::PluginHealthStatus> deserialiseThreatHealth(
        const std::string& pluginThreatHealthJsonString)
    {
        std::map<std::string, PluginCommunication::PluginHealthStatus> threatHealthMap;
        try
        {
            nlohmann::json threatHealthJsonObj;
            threatHealthJsonObj = nlohmann::json::parse(pluginThreatHealthJsonString);
            for (auto& [key, val] : threatHealthJsonObj.items())
            {
                PluginCommunication::PluginHealthStatus pluginThreatHealthStatus;
                pluginThreatHealthStatus.healthType = val.at("healthType");
                pluginThreatHealthStatus.healthValue = val.at("healthValue");
                pluginThreatHealthStatus.displayName = val.at("displayName");
                threatHealthMap[key] = pluginThreatHealthStatus;
            }
        }
        catch (nlohmann::json::out_of_range& jsonEx)
        {
            LOGERROR(
                "Key missing from JSON in deserialiseThreatHealth: " << jsonEx.what() << ", exception id: " << jsonEx.id
                                                                     << ", content: " << pluginThreatHealthJsonString);
        }
        catch (nlohmann::json::parse_error& jsonEx)
        {
            LOGERROR(
                "Failed to parse JSON in deserialiseThreatHealth: " << jsonEx.what() << ", exception id: " << jsonEx.id
                                                                    << ", content: " << pluginThreatHealthJsonString);
        }
        catch (const std::exception& exception)
        {
            LOGERROR(
                "Unexpected error while parsing JSON in deserialiseThreatHealth: " << exception.what() << ", content: "
                                                                                   << pluginThreatHealthJsonString);
        }

        return threatHealthMap;
    }

} // namespace ManagementAgent::HealthStatusImpl
