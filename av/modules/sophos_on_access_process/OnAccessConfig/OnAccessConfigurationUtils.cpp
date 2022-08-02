/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OnAccessConfigurationUtils.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <thirdparty/nlohmann-json/json.hpp>

namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace sophos_on_access_process::OnAccessConfig
{
    std::string readConfigFile()
    {
        auto* sophosFsAPI =  Common::FileSystem::fileSystem();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        auto configPath = pluginInstall / "var/soapd_config.json";

        try
        {
            std::string onAccessJsonConfig = sophosFsAPI->readFile(configPath.string());
            LOGDEBUG("New on-access configuration: " << onAccessJsonConfig);
            return  onAccessJsonConfig;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to read on-access configuration, keeping existing configuration");
            return  "";
        }
    }

    OnAccessConfiguration parseOnAccessSettingsFromJson(const std::string& jsonString)
    {
        json parsedConfig;
        try
        {
            parsedConfig = json::parse(jsonString);

            OnAccessConfiguration configuration{};
            configuration.enabled = isSettingTrue(parsedConfig["enabled"]);
            configuration.excludeRemoteFiles = isSettingTrue(parsedConfig["excludeRemoteFiles"]);
            configuration.exclusions = parsedConfig["exclusions"].get<std::vector<std::string>>();


            std::string scanNetwork = isSettingTrue(parsedConfig["excludeRemoteFiles"]) ? "\"false\"" : "\"true\"";
            LOGINFO("On-access enabled: " << parsedConfig["enabled"]);
            LOGINFO("On-access scan network: " << scanNetwork);
            LOGINFO("On-access exclusions: " << parsedConfig["exclusions"]);

            return configuration;
        }
        catch (const json::parse_error& e)
        {
            LOGWARN("Failed to parse json configuration due to parse error, reason: " << e.what());
        }
        catch (const json::out_of_range & e)
        {
            LOGWARN("Failed to parse json configuration due to out of range error, reason: " << e.what());
        }
            catch (const json::type_error & e)
        {
            LOGWARN("Failed to parse json configuration due to type error, reason: " << e.what());
        }
        catch (const json::other_error & e)
        {
            LOGWARN("Failed to parse json configuration, reason: " << e.what());
        }

        return {};
    }

    bool isSettingTrue(const std::string& settingString)
    {
        if(settingString == "true")
        {
            return true;
        }

        //return false for any other possibility
        return false;
    }
}

