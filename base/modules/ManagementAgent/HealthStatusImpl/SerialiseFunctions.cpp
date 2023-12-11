// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "SerialiseFunctions.h"

#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "ManagementAgent/HealthStatusCommon/PluginHealthStatus.h"

#include <nlohmann/json.hpp>

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
        LOGDEBUG("Deserialising Threat Health JSON: " << pluginThreatHealthJsonString);
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

    std::pair<bool, std::string> compareAndUpdateOverallHealth(
        healthValue_t health,
        healthValue_t service,
        healthValue_t threatService,
        healthValue_t threat)
    {
        std::pair<bool, std::string> healthInfo = std::make_pair(false, "");
        nlohmann::json currentJson;
        currentJson["health"] = health;
        currentJson["service"] = service;
        currentJson["threatService"] = threatService;
        currentJson["threat"] = threat;
        healthInfo.second = currentJson.dump();
        auto fs = Common::FileSystem::fileSystem();
        std::string overallHealthJsonFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getOverallHealthFilePath();
        if (fs->isFile(overallHealthJsonFilePath))
        {
            try
            {
                nlohmann::json oldJson = nlohmann::json::parse(fs->readFile(overallHealthJsonFilePath));
                if (oldJson == currentJson)
                {
                    return healthInfo;
                }
            }
            catch (Common::FileSystem::IFileSystemException& exception)
            {
                LOGERROR("Could not load cached Overall Health: " << exception.what());
            }
            catch (nlohmann::json::parse_error& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not parse json file: " << overallHealthJsonFilePath
                             << " with error: " << ex.what();
                LOGERROR(errorMessage.str());
            }
        }
        else
        {
            // This is expected on the first ever time MA starts up.
            LOGDEBUG("Overall Health JSON file does not exist at: " << overallHealthJsonFilePath);
        }
        healthInfo.first = true;
        return healthInfo;
    }

} // namespace ManagementAgent::HealthStatusImpl
