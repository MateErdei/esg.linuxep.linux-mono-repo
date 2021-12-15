
#include <ManagementAgent/PluginCommunication/PluginHealthStatus.h>

#include <json.hpp>

namespace ManagementAgent::HealthStatusImpl
{
    std::string serialiseThreatHealth(
        const std::map<std::string, PluginCommunication::PluginHealthStatus>& pluginThreatDetectionHealth)
    {



        auto pluginHealthArray = nlohmann::json::array();
        for (const auto& [pluginName, pluginHealthStatus] : pluginThreatDetectionHealth)
        {

            nlohmann::json pluginHealthStatusJsonObj;
            pluginHealthStatusJsonObj[pluginName]["displayName"] = "EDR Plugin";
            pluginHealthStatusJsonObj[pluginName]["healthValue"] = "1";
            pluginHealthStatusJsonObj[pluginName]["healthType"] = 0;

//            nlohmann::json pluginHealthStatusJsonObj = { pluginName,
//                    { "displayName", pluginHealthStatus.displayName },
//                    { "healthType", pluginHealthStatus.healthType },
//                    { "healthValue", pluginHealthStatus.healthValue } };

            pluginHealthArray.push_back(pluginHealthStatusJsonObj);
        }
        return pluginHealthArray.dump();
    }

//    std::string deserialiseThreatHealth(
//        const std::map<std::string, PluginCommunication::PluginHealthStatus>& pluginThreatDetectionHealth)
//    {
//        nlohmann::json j({});
//        for (const auto& [pluginName, pluginHealthStatus] : pluginThreatDetectionHealth)
//        {
//            j.push_back({ pluginName,
//                            { "displayName", pluginHealthStatus.displayName },
//                            { "healthType", pluginHealthStatus.healthType },
//                            { "healthValue", pluginHealthStatus.healthValue } });
//        }
//        return j.dump();
//    }
} // namespace ManagementAgent::HealthStatusImpl
