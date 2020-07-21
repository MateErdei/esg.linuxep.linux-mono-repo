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
        static std::tuple<std::string, std::string,std::string> readCurrentProxyInfo();
        const std::map<std::string,std::string> getKeyValue() const;
        const std::map<std::string,std::vector<std::string>> getKeyList() const;
        void addKeyToValue(std::pair<std::string,std::string>);
        void addKeyToList(std::pair<std::string,std::vector<std::string>>);
        CommsConfig(){addProxyInfoToConfig();};
        ~CommsConfig() = default;

    private:
        void addProxyInfoToConfig();
        std::map<std::string,std::string> m_key_value_config;
        std::map<std::string, std::vector<std::string>> m_key_composed_values_config;
    };
}