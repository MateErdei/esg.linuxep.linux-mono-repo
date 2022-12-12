// Copyright 2022 Sophos Limited. All rights reserved.

#include "OnAccessConfigurationUtils.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "thirdparty/nlohmann-json/json.hpp"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <limits>

namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace
{
    bool toBoolean(const json::value_type& value)
    {
        if (value.is_boolean())
        {
            return value.get<bool>();
        }
        if (value.is_string())
        {
            return sophos_on_access_process::OnAccessConfig::isSettingTrue(value.get<std::string>());
        }
        if (value.is_number())
        {
            return value.get<int>() != 0;
        }
        throw std::runtime_error("Can't convert JSON value to boolean");
    }

    bool toBoolean(const json& parsedConfig, const std::string& key, bool defaultValue)
    {
        try
        {
            return toBoolean(parsedConfig.at(key));
        }
        catch (const json::out_of_range&)
        {
            return defaultValue;
        }
    }

    template<typename T>
    T toLimitedInteger(const json& parsedConfigJson,
                       const std::string& key,
                       const std::string& name,
                       const T defaultValue,
                       const T minValue,
                       const T maxValue)
    {
        assert(minValue <= maxValue);
        assert(defaultValue <= maxValue);
        assert(minValue <= defaultValue);

        auto value = parsedConfigJson.value(key, defaultValue);

        if (value > maxValue)
        {
            LOGDEBUG(name << " of " << value << " is greater than maximum allowed of " << maxValue);
            return maxValue;
        }
        if (value < minValue)
        {
            LOGDEBUG(name << " of " << value << " is less than minimum allowed of " << minValue);
            return minValue;
        }
        return value;
    }
}

namespace sophos_on_access_process::OnAccessConfig
{
    fs::path policyConfigFilePath()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        return pluginInstall / "var/on_access_policy.json";
    }

    std::string readPolicyConfigFile(const fs::path& configPath)
    {
        auto* sophosFsAPI = Common::FileSystem::fileSystem();

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

    std::string readPolicyConfigFile()
    {
        return readPolicyConfigFile(policyConfigFilePath());
    }

    OnAccessLocalSettings readLocalSettingsFile(const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls)
    {
        OnAccessLocalSettings settings{};

        //may return 0 when not able to detect
        auto hardwareConcurrency = sysCalls->hardware_concurrency();
        if (hardwareConcurrency > 0)
        {
            assert(hardwareConcurrency < std::numeric_limits<int>::max());
            // Add 1 so that the result is rounded up and never less than 1
            int numScanThreads = (static_cast<int>(hardwareConcurrency) + 1) / 2;
            LOGDEBUG("Number of scanning threads set to " << numScanThreads);
            assert(numScanThreads > 0);
            settings.numScanThreads = numScanThreads;
        }
        else
        {
            LOGDEBUG("Could not determine hardware concurrency. Number of scanning threads defaulting to " << settings.numScanThreads);
        }


        auto* sophosFsAPI = Common::FileSystem::fileSystem();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        auto productConfigPath = pluginInstall / "var/on_access_local_settings.json";

        bool usedFileValues = false;

        try
        {
            std::string configJson = sophosFsAPI->readFile(productConfigPath.string());
            if (!configJson.empty())
            {
                try
                {
                    auto parsedConfigJson = json::parse(configJson);
                    settings.dumpPerfData = toBoolean(parsedConfigJson, "dumpPerfData", settings.dumpPerfData);
                    settings.cacheAllEvents = toBoolean(parsedConfigJson, "cacheAllEvents", settings.cacheAllEvents);
                    settings.uncacheDetections = toBoolean(parsedConfigJson, "uncacheDetections", settings.uncacheDetections);

                    settings.maxScanQueueSize = toLimitedInteger(
                            parsedConfigJson,
                            "maxscanqueuesize",
                            "Queue size",
                            settings.maxScanQueueSize,
                            minAllowedQueueSize,
                            maxAllowedQueueSize
                        );

                    settings.numScanThreads = toLimitedInteger(
                        parsedConfigJson,
                        "numThreads",
                        "Scanning Thread count",
                        settings.numScanThreads, // Dynamic from CPU core count
                        minAllowedScanningThreads,
                        maxAllowedScanningThreads
                        );

                    usedFileValues = true;
                }
                catch (const std::exception& e)
                {
                    LOGWARN("Failed to read product config file info: " << e.what());
                }
            }
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
        }
        std::string logmsg = usedFileValues ? "Setting from file: " : "Setting from defaults: ";

        LOGDEBUG(logmsg << "Max queue size set to " << settings.maxScanQueueSize << " and Max threads set to " << settings.numScanThreads);
        return settings;
    }

    std::string readFlagConfigFile()
    {
        auto* sophosFsAPI =  Common::FileSystem::fileSystem();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        auto flagPath = pluginInstall / "var/oa_flag.json";

        try
        {
            std::string onAccessFlagJson = sophosFsAPI->readFile(flagPath.string());
            LOGDEBUG("New flag configuration: " << onAccessFlagJson);
            return  onAccessFlagJson;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to read flag configuration, keeping existing configuration");
            return  "";
        }
    }

    bool parseFlagConfiguration(const std::string& jsonString)
    {
        try
        {
            json parsedConfig = json::parse(jsonString);
            return parsedConfig["oa_enabled"];
        }
        catch (const json::parse_error& e)
        {
            LOGDEBUG("Failed to parse json configuration of flags due to parse error, reason: " << e.what());
        }
        catch (const json::out_of_range & e)
        {
            LOGDEBUG("Failed to parse json configuration of flags due to out of range error, reason: " << e.what());
        }
        catch (const json::type_error & e)
        {
            LOGDEBUG("Failed to parse json configuration of flags due to type error, reason: " << e.what());
        }
        catch (const json::other_error & e)
        {
            LOGDEBUG("Failed to parse json configuration of flags, reason: " << e.what());
        }

        LOGWARN("Failed to parse flag configuration, keeping existing settings");

        return false;
    }

    OnAccessConfiguration parseOnAccessPolicySettingsFromJson(const std::string& jsonString)
    {
        json parsedConfig;
        try
        {
            parsedConfig = json::parse(jsonString);

            OnAccessConfiguration configuration{};
            configuration.enabled = toBoolean(parsedConfig, "enabled", false);
            LOGINFO("On-access enabled: " << (configuration.enabled ? "true" : "false")  << "");


            configuration.excludeRemoteFiles = toBoolean(parsedConfig, "excludeRemoteFiles", false);
            std::string scanNetwork = configuration.excludeRemoteFiles ? "false" : "true";
            LOGINFO("On-access scan network: " << scanNetwork);
            if (parsedConfig.contains("exclusions"))
            {
                for (const auto& exclusion : parsedConfig["exclusions"])
                {
                    configuration.exclusions.emplace_back(exclusion);
                }
                LOGINFO("On-access exclusions: " << parsedConfig["exclusions"]);
            }

            return configuration;
        }
        catch (const json::parse_error& e)
        {
            LOGDEBUG("Failed to parse json configuration due to parse error, reason: " << e.what());
        }
        catch (const json::out_of_range & e)
        {
            LOGDEBUG("Failed to parse json configuration due to out of range error, reason: " << e.what());
        }
            catch (const json::type_error & e)
        {
            LOGDEBUG("Failed to parse json configuration due to type error, reason: " << e.what());
        }
        catch (const json::other_error & e)
        {
            LOGDEBUG("Failed to parse json configuration, reason: " << e.what());
        }

        LOGWARN("Failed to parse json configuration, keeping existing settings");

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

