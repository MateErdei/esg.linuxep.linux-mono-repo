/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <map>
#include <vector>

namespace CommsComponent
{
    class CommsConfig
    {
    public:
        /**
        * Read a json file to get the proxy info
        * @param filepath, path for where the file containing the proxy info is located
        * @param fileSystem, to check if file exists
        * @return returns the address and password of proxy in a tuple
        */
        static std::tuple<std::string, std::string> readCurrentProxyInfo(const std::string& filepath);
        std::map<std::string, std::vector<std::string>> getKeyList() const;
        void addKeyValueToList(std::pair<std::string,std::vector<std::string>>);
        void addProxyInfoToConfig();
        
        CommsConfig() = default;
        ~CommsConfig() = default;

        void setProxy(const std::string & proxy); 
        void setDeobfuscatedCredential(const std::string & credential); 
        std::string getProxy() const; 
        std::string getDeobfuscatedCredential() const; 

    private:
        std::string getSimpleConfigValue(const std::string & key) const; 
        std::map<std::string, std::vector<std::string>> m_key_composed_values_config;
    };
}