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

            return (value.first == "0");;
        }
        else
        {
            LOGWARN("Could not find EDR Plugin config file: " << configPath);
        }
        throw std::runtime_error(flag + " not set in plugin setting file");
    }

    void PluginUtils::setGivenFlagFromSettingsFile(const std::string& flag, const bool& isXDR)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string mode;
        if (isXDR)
        {
            mode = flag + "=0";
        }
        else
        {
            mode = flag + "=1";
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

}