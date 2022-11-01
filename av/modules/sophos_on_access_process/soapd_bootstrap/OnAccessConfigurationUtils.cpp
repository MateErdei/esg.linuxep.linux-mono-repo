// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessConfigurationUtils.h"
#include "OnAccessProductConfigDefaults.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "thirdparty/nlohmann-json/json.hpp"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace sophos_on_access_process::OnAccessConfig
{
    fs::path policyConfigFilePath()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        return pluginInstall / "var/soapd_config.json";
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

    void readProductConfigFile(size_t& maxScanQueueSize, int& maxNumberOfScanThread, bool& dumpPerfData)
    {
        maxScanQueueSize = defaultMaxScanQueueSize;
        maxNumberOfScanThread = defaultScanningThreads;
        dumpPerfData = defaultDumpPerfData;

        auto* sophosFsAPI = Common::FileSystem::fileSystem();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        auto productConfigPath = pluginInstall / "var/onaccessproductconfig.json";

        bool usedFileValues = false;

        try
        {
            std::string configJson = sophosFsAPI->readFile(productConfigPath.string());
            if (!configJson.empty())
            {
                try
                {
                    auto parsedConfigJson = json::parse(configJson);
                    maxScanQueueSize = parsedConfigJson.value("maxscanqueuesize", defaultMaxScanQueueSize);
                    maxNumberOfScanThread = parsedConfigJson.value("maxthreads", defaultScanningThreads);
                    dumpPerfData = parsedConfigJson.value("dumpPerfData", defaultDumpPerfData);

                    if (maxScanQueueSize > maxAllowedQueueSize)
                    {
                        LOGDEBUG("Queue size of " << maxScanQueueSize << " is greater than maximum allowed of " << maxAllowedQueueSize);
                        maxScanQueueSize = maxAllowedQueueSize;
                    }
                    if (maxScanQueueSize < minAllowedQueueSize)
                    {
                        LOGDEBUG("Queue size of " << maxScanQueueSize << " is less than minimum allowed of " << minAllowedQueueSize);
                        maxScanQueueSize = minAllowedQueueSize;
                    }
                    if (maxNumberOfScanThread > maxAllowedScanningThreads)
                    {
                        LOGDEBUG("Scanning Thread count of " << maxNumberOfScanThread << " is greater than maximum allowed of " << maxAllowedScanningThreads);
                        maxNumberOfScanThread = maxAllowedScanningThreads;
                    }
                    if (maxNumberOfScanThread < minAllowedScanningThreads)
                    {
                        LOGDEBUG("Scanning Thread count of " << maxNumberOfScanThread << " is less than minimum allowed of " << minAllowedScanningThreads);
                        maxNumberOfScanThread = minAllowedScanningThreads;
                    }


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
        LOGDEBUG(logmsg << "Max queue size set to " << maxScanQueueSize << " and Max threads set to " << maxNumberOfScanThread);
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
            auto enabled = parsedConfig.value("enabled", "false");
            configuration.enabled = isSettingTrue(enabled);
            LOGINFO("On-access enabled: \"" << enabled << "\"");
            configuration.excludeRemoteFiles = isSettingTrue(parsedConfig.value("excludeRemoteFiles", "false"));
            std::string scanNetwork = configuration.excludeRemoteFiles ? "\"false\"" : "\"true\"";
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

