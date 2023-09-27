// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "OnAccessConfigurationUtils.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

// Third party
#include <nlohmann/json.hpp>

// System
#include <limits>

namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace
{

    [[maybe_unused]] std::string booleanToString(const bool& value)
    {
        return value ? "true" : "false";
    }

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
            auto res = toBoolean(parsedConfig.at(key));
            LOGDEBUG("Setting " << key << " from file: " << booleanToString(res));
            return res;
        }
        catch (const json::out_of_range&)
        {
            LOGDEBUG("Setting " << key << " from default: " << booleanToString(defaultValue));
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

        std::string source = value == defaultValue ? " from default: " : " from file: ";
        LOGDEBUG("Setting " << name << source << value);

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
            LOGWARN("Failed to read on-access configuration, keeping existing configuration: " << ex.what());
            return  "";
        }
    }

    std::string readPolicyConfigFile()
    {
        return readPolicyConfigFile(policyConfigFilePath());
    }

    OnAccessLocalSettings readLocalSettingsFile(const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr & sysCalls)
    {
        OnAccessLocalSettings settings{};

        auto* sophosFsAPI = Common::FileSystem::fileSystem();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        auto productConfigPath = pluginInstall / "var/on_access_local_settings.json";

        bool noLocalSettingsFile = false;
        bool threadsSetLocally = false;

        try
        {
            std::string configJson = sophosFsAPI->readFile(productConfigPath.string());

            if (!configJson.empty())
            {
                try
                {
                    auto parsedConfigJson = json::parse(configJson);

                    if (!parsedConfigJson.empty())
                    {
                        settings.dumpPerfData = toBoolean(parsedConfigJson, "dumpPerfData", settings.dumpPerfData);
                        settings.cacheAllEvents = toBoolean(parsedConfigJson, "cacheAllEvents", settings.cacheAllEvents);
                        settings.uncacheDetections = toBoolean(parsedConfigJson, "uncacheDetections", settings.uncacheDetections);
                        settings.highPrioritySoapd = toBoolean(parsedConfigJson, "highPrioritySoapd", settings.highPrioritySoapd);
                        settings.highPriorityThreatDetector =
                            toBoolean(parsedConfigJson, "highPriorityThreatDetector", settings.highPriorityThreatDetector);

                        settings.maxScanQueueSize = toLimitedInteger(
                            parsedConfigJson,
                            "maxscanqueuesize",
                            "Queue size",
                            settings.maxScanQueueSize,
                            minAllowedQueueSize,
                            maxAllowedQueueSize
                        );

                        if (parsedConfigJson.contains("numThreads"))
                        {
                            settings.numScanThreads = toLimitedInteger(
                                parsedConfigJson,
                                "numThreads",
                                "Scanning Thread count",
                                settings.numScanThreads,
                                minConfigurableScanningThreads,
                                maxConfigurableScanningThreads);
                            threadsSetLocally = true;
                        }
                    }
                    else
                    {
                        noLocalSettingsFile = true;
                    }
                }
                catch (const std::exception& e)
                {
                    LOGWARN("Failed to read local settings: " << e.what());
                    noLocalSettingsFile = true;
                }
            }
            else
            {
                LOGDEBUG("Local Settings file is empty");
                noLocalSettingsFile = true;
            }
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGDEBUG("Local Settings file could not be read: " << ex.what());
            noLocalSettingsFile = true;
        }

        if (!threadsSetLocally)
        {
            settings.numScanThreads = numberOfThreadsFromConcurrency(sysCalls);
        }

        if (noLocalSettingsFile)
        {
            LOGDEBUG("Some or all local settings weren't set from file: " <<
                     "Queue Size: " << settings.maxScanQueueSize <<
                     ", Max threads: " << settings.numScanThreads <<
                     ", Perf dump: " << booleanToString(settings.dumpPerfData) <<
                     ", Cache all events: " << booleanToString(settings.cacheAllEvents) <<
                     ", Uncache detections: " << booleanToString(settings.uncacheDetections));
        }

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
            LOGWARN("Failed to read flag configuration, keeping existing configuration: " << ex.what());
            return  "";
        }
    }

    bool parseFlagConfiguration(const std::string& jsonString)
    {
        if (jsonString.empty())
        {
            // Avoid trying to parse empty string
            return false;
        }
        try
        {
            json parsedConfig = json::parse(jsonString);
            return parsedConfig["oa_enabled"];
        }
        catch (const json::parse_error& e)
        {
            LOGWARN("Failed to parse json configuration of flags due to parse error, reason: " << e.what());
        }
        catch (const json::out_of_range & e)
        {
            LOGWARN("Failed to parse json configuration of flags due to out of range error, reason: " << e.what());
        }
        catch (const json::type_error & e)
        {
            LOGWARN("Failed to parse json configuration of flags due to type error, reason: " << e.what());
        }
        catch (const json::other_error & e)
        {
            LOGWARN("Failed to parse json configuration of flags, reason: " << e.what());
        }

        LOGWARN("Failed to parse flag configuration, keeping existing settings");

        return false;
    }

    bool parseOnAccessPolicySettingsFromJson(const std::string& jsonString, OnAccessConfiguration& oaConfig)
    {
        if (jsonString.empty())
        {
            return false;
        }
        json parsedConfig;
        try
        {
            parsedConfig = json::parse(jsonString);

            if (parsedConfig.empty())
            {
                return false;
            }

            OnAccessConfiguration configuration{};
            configuration.enabled = toBoolean(parsedConfig, "enabled", false);
            LOGDEBUG("On-access enabled: " << (configuration.enabled ? "true" : "false")  << "");


            configuration.excludeRemoteFiles = toBoolean(parsedConfig, "excludeRemoteFiles", false);
            std::string scanNetwork = configuration.excludeRemoteFiles ? "false" : "true";
            LOGDEBUG("On-access scan network: " << scanNetwork);
            if (parsedConfig.contains("exclusions"))
            {
                for (const auto& exclusion : parsedConfig["exclusions"])
                {
                    configuration.exclusions.emplace_back(exclusion);
                }
                std::sort(configuration.exclusions.begin(), configuration.exclusions.end());
                LOGDEBUG("On-access exclusions: " << parsedConfig["exclusions"]);
            }

            configuration.detectPUAs = toBoolean(parsedConfig, "detectPUAs", true);
            LOGDEBUG("PUA detection enabled: " << (configuration.detectPUAs ? "true" : "false")  << "");

            configuration.onOpen = toBoolean(parsedConfig, "onOpen", true);
            if (configuration.onOpen)
            {
                LOGDEBUG("Scanning on-open enabled");
            }
            else
            {
                // Deliberately log at info level so that we can diagnose issues quicker
                LOGINFO("Scanning on-open disabled");
            }
            configuration.onClose = toBoolean(parsedConfig, "onClose", true);
            if (configuration.onClose)
            {
                LOGDEBUG("Scanning on-close enabled");
            }
            else
            {
                // Deliberately log at info level so that we can diagnose issues quicker
                LOGINFO("Scanning on-close disabled");
            }

            oaConfig = std::move(configuration);
            return true;
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
        catch (const std::exception& e)
        {
            LOGWARN("Failed to parse json configuration due to generic exception, reason:" << e.what());
        }

        LOGWARN("Failed to parse json configuration, keeping existing settings");

        return false;
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

    int numberOfThreadsFromConcurrency(const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr & sysCalls)
    {
        int threadsFromConcurrency = defaultScanningThreads;
        //may return 0 when not able to detect
        auto hardwareConcurrency = sysCalls->hardware_concurrency();
        if (hardwareConcurrency > 0)
        {
            assert(hardwareConcurrency < std::numeric_limits<int>::max());
            // Add 1 so that the result is rounded up and never less than 1
            int numScanThreads = (static_cast<int>(hardwareConcurrency) + 1) / 2;
            assert(numScanThreads > 0);
            if (numScanThreads > maxConcurrencyScanningThreads)
            {
                LOGDEBUG("Hardware concurrency result is " << numScanThreads << " which is to high. Reducing number of threads to " << maxConcurrencyScanningThreads);
                threadsFromConcurrency = maxConcurrencyScanningThreads;
            }
            else if (numScanThreads < minConcurrencyScanningThreads)
            {
                LOGDEBUG("Hardware concurrency result is " << numScanThreads << " which is to low. Increasing number of threads to " << maxConcurrencyScanningThreads);
                threadsFromConcurrency = minConcurrencyScanningThreads;
            }
            else
            {
                LOGDEBUG("Setting number of scanning threads from Hardware Concurrency: " << numScanThreads);
                threadsFromConcurrency = numScanThreads;
            }
        }
        else
        {
            LOGDEBUG("Could not determine hardware concurrency using default of " << defaultScanningThreads);
        }
        return threadsFromConcurrency;
    }

}

