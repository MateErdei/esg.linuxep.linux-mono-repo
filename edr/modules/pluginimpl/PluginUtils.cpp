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
    void PluginUtils::setFlagInPluginConfigFile(const std::string& flag, const bool& flagValue)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string settingString;
        if (flagValue)
        {
            settingString = flag + "=1";
        }
        else
        {
            settingString = flag + "=0";
        }

        try
        {
            std::string configPath = Plugin::edrConfigFilePath();
            if (fileSystem->isFile(configPath))
            {
                bool notSet = true;
                std::vector<std::string> content = fileSystem->readLines(configPath);
                std::stringstream newContent;
                for (const auto &line : content)
                {
                    if (Common::UtilityImpl::StringUtils::isSubstring(line, flag))
                    {
                        newContent << settingString << "\n";
                        notSet = false;
                    }
                    else
                    {
                        newContent << line << "\n";
                    }
                }

                // if setting not already set, append to end of content
                if (notSet)
                {
                    newContent << settingString << "\n";
                }

                fileSystem->writeFileAtomically(configPath, newContent.str(), Plugin::etcDir());
            }
            else
            {
                fileSystem->writeFile(configPath, settingString + "\n");
            }
            LOGINFO("Updated plugin conf with new " + flag);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to update plugin conf with error: " << ex.what());
        }
    }

}