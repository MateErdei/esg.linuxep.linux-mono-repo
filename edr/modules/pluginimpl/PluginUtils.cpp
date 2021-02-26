/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "PluginUtils.h"
#include "ApplicationPaths.h"
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/FileSystem/IFileSystemException.h>

namespace Plugin
{
    bool PluginUtils::isFlagSet(const std::string& flag, const std::string& flagContent)
    {
        bool flagValue = false;

        try
        {
            nlohmann::json j = nlohmann::json::parse(flagContent);

            if (j.find(flag) != j.end())
            {
                if (j[flag] == true)
                {
                    flagValue = true;
                }
            }
        }
        catch (nlohmann::json::parse_error& ex)
        {
            std::stringstream errorMessage;
            errorMessage << "Could not parse json: " << flagContent << " with error: " << ex.what();
            LOGWARN(errorMessage.str());
        }

        return flagValue;
    }

    bool PluginUtils::retrieveGivenFlagFromSettingsFile(const std::string& flag)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string configPath = Plugin::edrConfigFilePath();

        if (fileSystem->isFile(configPath))
        {
            std::pair<std::string,std::string> value = Common::UtilityImpl::FileUtils::extractValueFromFile(configPath, flag);
            if (value.first.empty() && !value.second.empty())
            {
                LOGWARN("Failed to read " << flag << " configuration from config file due to error: " << value.second);
                throw std::runtime_error(flag + " not set in plugin setting file");
            }

            return (value.first == "1");;
        }
        else
        {
            LOGWARN("Could not find EDR Plugin config file: " << configPath);
        }
        throw std::runtime_error(flag + " not set in plugin setting file");
    }

    bool PluginUtils::isInteger(const std::string& number)
    {
        if (number.find_first_not_of("1234567890") == std::string::npos)
        {
            return true;
        }
        return false;
    }
    void PluginUtils::setGivenFlagFromSettingsFile(const std::string& flag, const bool& flagValue)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string mode;
        if (flagValue)
        {
            mode = flag + "=1";
        }
        else
        {
            mode = flag + "=0";
        }

        try
        {
            std::string configPath = Plugin::edrConfigFilePath();
            if (fileSystem->isFile(configPath))
            {
                bool modeIsNotSet = true;
                std::vector<std::string> content = fileSystem->readLines(configPath);
                std::stringstream newContent;
                for (const auto &line : content)
                {
                    // if running mode already set replace it
                    if (Common::UtilityImpl::StringUtils::isSubstring(line, flag))
                    {
                        newContent << mode << "\n";
                        modeIsNotSet = false;
                    }
                    else
                        {
                        newContent << line << "\n";
                    }
                }

                // if running mode not already set append to end of content
                if (modeIsNotSet)
                {
                    newContent << mode << "\n";
                }

                fileSystem->writeFileAtomically(configPath, newContent.str(), Plugin::etcDir());
            }
            else
            {
                fileSystem->writeFile(configPath, mode + "\n");
            }
            LOGINFO("Updated plugin conf with new " + flag);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to update plugin conf with error: " << ex.what());
        }
    }

    std::pair<std::string, std::string> PluginUtils::getRunningQueryPackFilePaths()
    {
        auto* ifileSystem = Common::FileSystem::fileSystem();
        std::string targetDirectoryPath = Plugin::osqueryConfigDirectoryPath();
        std::string targetMtrFilePath = Plugin::osqueryMTRConfigFilePath();
        std::string targetXdrFilePath = Plugin::osqueryXDRConfigFilePath();

        try
        {
            if (ifileSystem->isDirectory(targetDirectoryPath))
            {
                std::vector<std::string> paths = ifileSystem->listFiles(targetDirectoryPath);

                if (!paths.empty())
                {
                    for (const auto& filepath : paths)
                    {
                        std::string basename = Common::FileSystem::basename(filepath);
                        if (Common::UtilityImpl::StringUtils::startswith(basename, "sophos-scheduled-query-pack"))
                        {
                            if (Common::UtilityImpl::StringUtils::isSubstring(basename, "mtr"))
                            {
                                targetMtrFilePath = filepath;
                            }
                            else if (!Common::UtilityImpl::StringUtils::isSubstring(basename, "custom"))
                            {
                                targetXdrFilePath = filepath;
                            }
                        }
                    }
                }
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Cannot check for existing query pack names due to exception: " << e.what() << ". Using default filenames: "
            << Common::FileSystem::basename(targetMtrFilePath) << " & " << Common::FileSystem::basename(targetXdrFilePath));
        }
        return {targetMtrFilePath, targetXdrFilePath};
    }

    void PluginUtils::setQueryPacksInPlace(const bool& useNextQueryPack)
    {
        std::pair<std::string,std::string> targetFiles = getRunningQueryPackFilePaths();
        auto* ifileSystem = Common::FileSystem::fileSystem();

        try
        {
            if (useNextQueryPack)
            {
                LOGINFO("Overwriting existing scheduled query packs with 'NEXT' query packs");
                ifileSystem->copyFile(Plugin::osqueryNextMTRConfigStagingFilePath(), targetFiles.first);
                ifileSystem->copyFile(Plugin::osqueryNextXDRConfigStagingFilePath(), targetFiles.second);
            }
            else
            {
                LOGINFO("Reverting to existing scheduled query packs");
                ifileSystem->copyFile(Plugin::osqueryMTRConfigStagingFilePath(), targetFiles.first);
                ifileSystem->copyFile(Plugin::osqueryXDRConfigStagingFilePath(), targetFiles.second);
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Failed to set query packs to the correct version: " << e.what());
        }
    }

    void PluginUtils::updatePluginConfWithFlag(const std::string& flagName, const bool& flagSetting, bool& flagsHaveChanged)
    {
        try
        {
            bool oldMode = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(flagName);
            if (oldMode != flagSetting)
            {
                LOGINFO("Updating " << flagName << " flag settings");
                Plugin::PluginUtils::setGivenFlagFromSettingsFile(flagName, flagSetting);
                flagsHaveChanged = true;
            }
        }
        catch (const std::runtime_error& ex)
        {
            LOGINFO("Setting " << flagName << " flag settings");
            Plugin::PluginUtils::setGivenFlagFromSettingsFile(flagName, flagSetting);
            flagsHaveChanged = true;
        }
    }
}
