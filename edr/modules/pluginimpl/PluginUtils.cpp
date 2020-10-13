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

            if (j.find("xdr.enabled") != j.end())
            {
                if (j["xdr.enabled"] == "true")
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
        bool isXDR = false;
        std::string configpath = Plugin::edrConfigFilePath();
        if (fileSystem->isFile(configpath))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(configpath, ptree);
                isXDR = (ptree.get<std::string>(m_mode_identifier) == "1");
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
            mode = m_mode_identifier + "=0";
        }
        else
        {
            mode = m_mode_identifier + "=1";
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
                    if (Common::UtilityImpl::StringUtils::isSubstring(m_mode_identifier, line))
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

                std::string tempPath = Plugin::tempPluginConf();
                fileSystem->writeFile(tempPath, newContent);
                fileSystem->moveFile(tempPath, configpath);
            }
            else
            {
                fileSystem->writeFile(configpath, mode);
            }
            LOGINFO("Updated plugin conf with new running mode");
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to update plugin conf with error: " << ex.what());
        }
    }

}