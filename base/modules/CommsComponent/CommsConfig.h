/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <map>
#include <string>
#include <vector>

namespace CommsComponent
{
    class CommsConfig
    {
    public:
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