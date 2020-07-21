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
    std::tuple<std::string, std::string, std::string> CommsConfig::readCurrentProxyInfo() {

        std::string filepath = Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplConfigFileDirectory()
                , "current_proxy");
        auto fileSystem = Common::FileSystem::fileSystem();


        std::string proxy = "";
        std::string credentials = "";
        std::string username = "";
        if (fileSystem->isFile(filepath)) {
            std::ifstream my_file(filepath);

            try {
                nlohmann::json j = nlohmann::json::parse(my_file);

                if (j.find("proxy") != j.end()) {
                    proxy = j["proxy"];
                    if (j.find("credentials") != j.end()) {
                        username = j["username"];
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
        return {proxy, credentials, username};
    }

    void CommsConfig::addProxyInfoToConfig()
    {

        auto [proxy,credentials,username] = readCurrentProxyInfo();

        if (!proxy.empty())
        {
            LOGDEBUG("Writing proxy info to file");
            addKeyToValue(std::pair<std::string,std::string>("proxy",proxy));

            if (!credentials.empty())
            {
                LOGDEBUG("Writing credential info to file");
                auto deobfuscated = Common::ObfuscationImpl::SECDeobfuscate(credentials);
                addKeyToValue(std::pair<std::string,std::string>("username",username));
                addKeyToValue(std::pair<std::string,std::string>("password",deobfuscated));

            }
        }
    }
    const std::map<std::string,std::string> CommsConfig::getKeyValue() const { return m_key_value_config;}
    const std::map<std::string,std::vector<std::string>> CommsConfig::getKeyList() const { return m_key_composed_values_config;}



    void CommsConfig::addKeyToValue(std::pair<std::string,std::string> keyValue){ m_key_value_config.insert(keyValue);}
    void CommsConfig::addKeyToList(std::pair<std::string,std::vector<std::string>> keyList){m_key_composed_values_config.insert(keyList);}
}