// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "PluginUtils.h"

#include <nlohmann/json.hpp>


#include "Common/UtilityImpl/StringUtils.h"
#include <fstream>
namespace Plugin
{
    std::tuple<std::string, std::string> PluginUtils::readCurrentProxyInfo(const std::string& filepath, Common::FileSystem::IFileSystem *fileSystem)
    {
        std::string proxy = "";
        std::string credentials = "";
        if (fileSystem->isFile(filepath))
        {
            std::ifstream my_file(filepath);

            try
            {
                nlohmann::json j = nlohmann::json::parse(my_file);

                if (j.find("proxy") != j.end())
                {
                    proxy = j["proxy"];
                    if (j.find("credentials") != j.end())
                    {
                        credentials = j["credentials"];
                    }
                }
            }
            catch (nlohmann::json::parse_error &ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not parse json file: " << filepath << " with error: " << ex.what();
                LOGWARN(errorMessage.str());
            }
        }
        return {proxy, credentials};
    }

    std::string PluginUtils::getHostnameFromUrl(const std::string& url)
    {
        std::string hostname = url;

        if (hostname.find("https://") == 0)
        {
            hostname = hostname.substr(8);
        }
        if (hostname.find("http://") == 0)
        {
            hostname = hostname.substr(7);
        }
        if (hostname.find("/") != std::string::npos)
        {
            hostname = hostname.substr(0,hostname.find("/"));
        }
        return hostname;
    }

    std::optional<std::string> PluginUtils::readKeyValueFromConfig(const std::string& key, const std::string& filepath)
    {

            try
            {
                std::string value = Common::UtilityImpl::StringUtils::extractValueFromConfigFile(filepath,key);
                if (!value.empty())
                {
                    return value;
                }
            }
            catch (std::runtime_error&)
            {
                LOGWARN(filepath << " does not exist so could not read " << key);
                return std::nullopt;
            }


        return std::nullopt;
    }

    std::optional<std::string> PluginUtils::readKeyValueFromIniFile(const std::string& key, const std::string& filepath)
    {

        try
        {
            std::string value = Common::UtilityImpl::StringUtils::extractValueFromIniFile(filepath,key);
            if (!value.empty())
            {
                return value;
            }
        }
        catch (std::runtime_error&)
        {
            LOGWARN(filepath << " does not exist so could not read " << key);
            return std::nullopt;
        }


        return std::nullopt;
    }

    std::string PluginUtils::getVersionOS()
    {
        auto version = Common::UtilityImpl::FileUtils::extractValueFromFile("/etc/os-release", "PRETTY_NAME");
        if (!version.first.empty())
        {
            // The PRETTY_NAME has " and we remove it
            std::string trimmedVersion = version.first.substr(1, version.first.size()-2);
            return trimmedVersion;
        }
        // This is the default value in case there is something wrong with the OS Name
        return "Linux";
    }
}