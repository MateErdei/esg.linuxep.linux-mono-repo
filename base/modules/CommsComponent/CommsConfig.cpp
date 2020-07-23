/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ObfuscationImpl/Obfuscate.h>

#include "CommsConfig.h"
#include "Logger.h"
#include <json.hpp>

#include <string>
#include <fstream>

namespace CommsComponent
{
    std::tuple<std::string, std::string> CommsConfig::readCurrentProxyInfo(const std::string filepath) {

        auto fileSystem = Common::FileSystem::fileSystem();

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
            catch (nlohmann::json::parse_error &ex) {
                std::stringstream errorMessage;
                errorMessage << "Could not parse json file: " << filepath << " with error: " << ex.what();
                LOGWARN(errorMessage.str());
            }
        }
        return {proxy, credentials};
    }

    void CommsConfig::addProxyInfoToConfig()
    {
        std::string filepath = Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory()
                , "current_proxy");
        auto [proxy,credentials,username] = readCurrentProxyInfo(filepath);

        if (!proxy.empty())
        {
            LOGDEBUG("Storing proxy info");
            addKeyValueToList(std::pair<std::string,std::vector<std::string>>("proxy",{proxy}));

            if (!credentials.empty())
            {
                LOGDEBUG("Storing credential info");
                auto deobfuscated = Common::ObfuscationImpl::SECDeobfuscate(credentials);
                addKeyValueToList(std::pair<std::string,std::vector<std::string>>("credentials",{deobfuscated}));
            }
        }
    }
    std::map<std::string, std::vector<std::string>> CommsConfig::getKeyList() const
    {
        return m_key_composed_values_config;
    }

    void CommsConfig::addKeyValueToList(std::pair<std::string,std::vector<std::string>> keyList)
    {
        m_key_composed_values_config.insert(keyList);
    }
}