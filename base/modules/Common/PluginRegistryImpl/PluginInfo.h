/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_PLUGINREGISTRY_H
#define EVEREST_BASE_PLUGINREGISTRY_H

#include <string>
#include <vector>


namespace Common
{
    namespace PluginRegistryImpl
    {
        class PluginInfo
        {
        public:
            using EnvPairs = std::vector<std::pair<std::string, std::string>>;
            std::vector<std::string> getAppIds() const;
            std::string getPluginName() const;
            std::string getPluginIpcAddress() const;
            std::string getXmlTranslatorPath() const;
            std::string getExecutableFullPath() const;
            std::vector<std::string> getExecutableArguments() const;
            EnvPairs getExecutableEnvironmentVariables() const;

            void setAppIds( const std::vector<std::string> & appIDs);
            void addAppIds( const std::string & appID);
            void setPluginName(const std::string &pluginName);
            void setXmlTranslatorPath(const std::string &xmlTranslationPath);
            void setExecutableFullPath(const std::string &executableFullPath);
            void setExecutableArguments(const std::vector<std::string> &executableArguments);
            void addExecutableArguments(const std::string &executableArgument);
            void setExecutableEnvironmentVariables(const EnvPairs  &executableEnvironmentVariables);
            void addExecutableEnvironmentVariables(const std::string& environmentName, const std::string &environmentValue);

            static PluginInfo deserializeFromString(const std::string &);
            static std::vector<PluginInfo> loadFromDirectoryPath(const std::string & );

        private:
            std::vector<std::string> m_appIds;
            std::string m_pluginName;
            std::string m_xmlTranslatorPath;
            std::string m_executableFullPath;
            std::vector<std::string> m_executableArguments;
            EnvPairs m_executableEnvironmentVariables;

        };
    }
}

#endif //EVEREST_BASE_PLUGINREGISTRY_H
