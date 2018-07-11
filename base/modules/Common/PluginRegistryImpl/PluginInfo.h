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
            std::vector<std::string> getPolicyAppIds() const;
            std::vector<std::string> getStatusAppIds() const;
            std::string getPluginName() const;
            std::string getPluginIpcAddress() const;
            std::string getXmlTranslatorPath() const;
            std::string getExecutableFullPath() const;
            std::vector<std::string> getExecutableArguments() const;
            EnvPairs getExecutableEnvironmentVariables() const;

            void setPolicyAppIds(const std::vector<std::string> &appIDs);
            void addPolicyAppIds(const std::string &appID);
            void setStatusAppIds(const std::vector<std::string> &appIDs);
            void addStatusAppIds(const std::string &appID);
            void setPluginName(const std::string &pluginName);
            void setXmlTranslatorPath(const std::string &xmlTranslationPath);
            void setExecutableFullPath(const std::string &executableFullPath);
            void setExecutableArguments(const std::vector<std::string> &executableArguments);
            void addExecutableArguments(const std::string &executableArgument);
            void setExecutableEnvironmentVariables(const EnvPairs  &executableEnvironmentVariables);
            void addExecutableEnvironmentVariables(const std::string& environmentName, const std::string &environmentValue);

            /**
             * Serialize pluginInfo object into protobuf message.
             * @param pluginInfo object to be serilized
             * @return pluginInfo represented as protobuf object.
             */
            static std::string serializeToString(const PluginInfo &pluginInfo);

            /**
             * Deserialize a Json containing the PluginInfo into the PluginInfo object.
             *
             * @throws PluginRegistryException if the json content can not be safely deserialized into the PluginInfo.
             *         It does not provide guarantee that the values for AppIds, ExecutablePaths, etc are correct to use,
             *         The exception is thrown only to signal that the content of the json was malformed and could not be parsed.
             *         It is the responsibility of the whoever used PluginInfo to verify that the content is 'Adequate' to be used.
             *
             * @return PluginInfo parsed from the serializedPluginInfo.
             */
            static PluginInfo deserializeFromString(const std::string & serializedPluginInfo);
            /**
             * List the json entries from the directoryPath and load them into a vector of PluginInfo.
             * Failed to load specific entries will be 'silently' ignored. Although a log warning will be produced.
             *
             */
            static std::vector<PluginInfo> loadFromDirectoryPath(const std::string & directoryPath);

            /**
             * Uses the expected location of the plugin registry folder to load the plugins information.
             *
             * It is equivalent to:
             * @code
             * return loadFromDirectoryPath(<installroot>/pluginRegistry)
             * @endcode
             * @return
             */
            static std::vector<PluginInfo> loadFromPluginRegistry();


        private:
            std::vector<std::string> m_policyAppIds;
            std::vector<std::string> m_statusAppIds;
            std::string m_pluginName;
            std::string m_xmlTranslatorPath;
            std::string m_executableFullPath;
            std::vector<std::string> m_executableArguments;
            EnvPairs m_executableEnvironmentVariables;

        };
    }
}

#endif //EVEREST_BASE_PLUGINREGISTRY_H
