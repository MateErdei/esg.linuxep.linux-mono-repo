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

namespace
{
    const char * GL_PROXY = "proxy"; 
    const char * GL_CRED = "credentials"; 
}


namespace CommsComponent
{
    std::tuple<std::string, std::string> CommsConfig::readCurrentProxyInfo(const std::string& filepath) {

        auto fileSystem = Common::FileSystem::fileSystem();

        std::string proxy = "";
        std::string credentials = "";

        if (fileSystem->isFile(filepath))
        {
            std::string fileContents = fileSystem->readFile(filepath);
            try
            {
                nlohmann::json j = nlohmann::json::parse(fileContents);

                if (j.find(GL_PROXY) != j.end())
                {
                    proxy = j[GL_PROXY];
                    if (j.find(GL_CRED) != j.end())
                    {
                        credentials = j[GL_CRED];
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
        auto [proxy,credentials] = readCurrentProxyInfo(filepath);

        if (!proxy.empty())
        {
            setProxy(proxy); 
            if (!credentials.empty())
            {                
                auto deobfuscated = Common::ObfuscationImpl::SECDeobfuscate(credentials);
                setDeobfuscatedCredential(deobfuscated); 
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

    void CommsConfig::setProxy(const std::string & proxy)
    {
        LOGDEBUG("Storing proxy info");
        addKeyValueToList(std::pair<std::string,std::vector<std::string>>(GL_PROXY,{proxy}));

    }
    void CommsConfig::setDeobfuscatedCredential(const std::string & credential)
    {
        LOGDEBUG("Storing credential info");
        addKeyValueToList(std::pair<std::string,std::vector<std::string>>(GL_CRED,{credential}));

    }
    std::string CommsConfig::getProxy() const
    {
        return getSimpleConfigValue(GL_PROXY); 
    }
    std::string CommsConfig::getDeobfuscatedCredential() const
    {
        return getSimpleConfigValue(GL_CRED); 
    }
    std::string  CommsConfig::getSimpleConfigValue(const std::string & key) const
    {
        auto found = m_key_composed_values_config.find(key); 
        if ( found == m_key_composed_values_config.end())
        {
            return std::string{}; 
        }
        if (found->second.empty())
        {
            return std::string{}; 
        }
        if (found->second.size()>1)
        {
            throw std::logic_error("The key does not refer to a simple string value"); 
        }
        return found->second.at(0); 
    }

}