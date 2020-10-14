/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "PluginUtils.h"
#include "ApplicationPaths.h"
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>
#include <redist/boost/include/boost/property_tree/ini_parser.hpp>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/FileSystem/IFileSystemException.h>

namespace Plugin
{
    bool PluginUtils::isRunningModeXDR(const std::string &flagContent)
    {
        bool isXDR = false;

        try
        {
            nlohmann::json j = nlohmann::json::parse(flagContent);

            if (j.find(XDR_FLAG) != j.end())
            {
                if (j[XDR_FLAG] == true)
                {
                    isXDR = true;
                }
            }
        }
        catch (nlohmann::json::parse_error &ex)
        {
            std::stringstream errorMessage;
            errorMessage << "Could not parse json: " << flagContent << " with error: " << ex.what();
            LOGWARN(errorMessage.str());
        }

        return isXDR;
    }

    bool PluginUtils::retrieveRunningModeFlagFromSettingsFile()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string configpath = Plugin::edrConfigFilePath();

        if (fileSystem->isFile(configpath))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(configpath, ptree);
                bool isXDR = (ptree.get<std::string>(MODE_IDENTIFIER) == "0");
                return isXDR;
            }
            catch (boost::property_tree::ptree_error& ex)
            {
                LOGWARN("Failed to read running mode configuration from config file");
            }
        }
        else
        {
            LOGWARN("Could not find EDR Plugin config file: " << configpath);
        }
        throw std::runtime_error("running mode not set in plugin setting file");
    }

    void PluginUtils::setRunningModeFlagFromSettingsFile(const bool &isXDR)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string mode;
        if (isXDR)
        {
            mode = MODE_IDENTIFIER + "=0";
        }
        else
        {
            mode = MODE_IDENTIFIER + "=1";
        }

        try
        {
            std::string configpath = Plugin::edrConfigFilePath();
            if (fileSystem->isFile(configpath))
            {
                bool modeIsNotSet = true;
                std::vector<std::string> content = fileSystem->readLines(configpath);
                std::string newContent;
                for (const auto &line : content)
                {
                    // if running mode already set replace it
                    if (Common::UtilityImpl::StringUtils::isSubstring(line, MODE_IDENTIFIER))
                    {
                        newContent = newContent + mode + "\n";
                        modeIsNotSet = false;
                    }
                    else
                        {
                        newContent = newContent + line + "\n";
                    }
                }

                // if running mode not already set append to end of content
                if (modeIsNotSet)
                {
                    newContent = newContent + mode + "\n";
                }


                fileSystem->writeFileAtomically(configpath,newContent,Plugin::etcDir());
            }
            else
            {
                fileSystem->writeFile(configpath, mode+ "\n");
            }
            LOGINFO("Updated plugin conf with new running mode");
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to update plugin conf with error: " << ex.what());
        }
    }

}