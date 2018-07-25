/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef COMMON_PLUGINREGISTRYIMPL_PLUGININFO_H
#define COMMON_PLUGINREGISTRYIMPL_PLUGININFO_H

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/Process/EnvPair.h>

#include <string>
#include <vector>



namespace Common
{
    namespace PluginRegistryImpl
    {
        class PluginInfo
        {
        public:
            using EnvPairs = Common::Process::EnvPairVector;

            /**
             * Used to get stored policy appIds
             * @return list of app ids for the policies the plugin will support
             */
            std::vector<std::string> getPolicyAppIds() const;
            /**
             * Used to get stored status appIds
             * @return list of app ids for the statuses the plugin will support
             */
            std::vector<std::string> getStatusAppIds() const;

            /**
             * Used to get the name of the plugin the information is associated with
             * @return string containing the plugin name.
             */
            std::string getPluginName() const;

            /**
             * Used to get the address used for the plugin IPC connection.
             * @return string containing the IPC address for the plugin
             */
            std::string getPluginIpcAddress() const;

            /**
             * Used to get the path of the xml library used to provide xml translation for the plugin.
             * @return string containing a file path.
             */
            std::string getXmlTranslatorPath() const;

            /**
             * Used to get the path of the plugin executable.
             * @return string containing the path
             */
            std::string getExecutableFullPath() const;

            /**
             * Used to get the required arguments for the plugin executable (if any)
             * @return zero or more strings used for the executable arguments.
             */
            std::vector<std::string> getExecutableArguments() const;

            /**
             * Used to get the required environment variables for the plugin executable (if any)
             * @return zero or more strings used for the executable environment variables.
             */
            EnvPairs getExecutableEnvironmentVariables() const;

            /**
             * Used to store the given Policy AppIds the plugin is interested in.
             * @param list of appIDs
             */
            void setPolicyAppIds(const std::vector<std::string> &appIDs);

            /**
             * Used to add a single Policy appId to the stored PolicyAppId list.
             * @param appID
             */
            void addPolicyAppIds(const std::string &appID);

            /**
             * Used to store the given Status AppIds the plugin is interested in.
             * @param list of appIDs
             */
            void setStatusAppIds(const std::vector<std::string> &appIDs);

            /**
             * Used to add a single status appId to the stored StatusAppId list.
             * @param appID
             */
            void addStatusAppIds(const std::string &appID);

            /**
             * Used to store the given plugin name
             * @param pluginName
             */
            void setPluginName(const std::string &pluginName);

            /**
             * Used to store the given path for the location of the XML translator used by the plugin
             * @param xmlTranslationPath
             */
            void setXmlTranslatorPath(const std::string &xmlTranslationPath);

            /**
             * Used to store the full part to the plugin executable
             * @param executableFullPath
             */
            void setExecutableFullPath(const std::string &executableFullPath);

            /**
            * Used to store the given arguments the plugin requires to run.
            * @param list of executableArguments
            */
            void setExecutableArguments(const std::vector<std::string> &executableArguments);

            /**
            * Used to add the given argument to the stored list of arguments.
            * @param executableArgument
            */
            void addExecutableArguments(const std::string &executableArgument);

            /**
             * Used to store a list of environment variables required by the plugin.
             * @param executableEnvironmentVariables
             */
            void setExecutableEnvironmentVariables(const EnvPairs  &executableEnvironmentVariables);

            /**
             * Used to add a single environment variable to the list of stored environment variables required by the plugin.
             * @param environmentName
             * @param environmentValue
             */
            void addExecutableEnvironmentVariables(const std::string& environmentName, const std::string &environmentValue);

            /**
             * Serialize pluginInfo object into protobuf message.
             * @param pluginInfo object to be serialized
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

            using PluginInfoVector = std::vector<PluginInfo>;

            /**
             * List the json entries from the directoryPath and load them into a vector of PluginInfo.
             * Failed to load specific entries will be 'silently' ignored. Although a log warning will be produced.
             *
             * @code
             * Uses deserializeFromString to convert json files into PluginInfo objects
             * @return A list of pluginInfo objects containing the loaded from the files read from disk.
             */
            static PluginInfoVector loadFromDirectoryPath(const std::string & directoryPath);

            /**
             * Uses the expected location of the plugin registry folder to load the plugins information.
             *
             * It is equivalent to:
             * @code
             * return loadFromDirectoryPath(<installroot>/base/pluginRegistry)
             * @endcode
             * @return A list of pluginInfo objects containing the loaded from the files read from disk.
             */
            static PluginInfoVector loadFromPluginRegistry();


        private:
            std::vector<std::string> m_policyAppIds;
            std::vector<std::string> m_statusAppIds;
            std::string m_pluginName;
            std::string m_xmlTranslatorPath;
            std::string m_executableFullPath;
            std::vector<std::string> m_executableArguments;
            EnvPairs m_executableEnvironmentVariables;


        };

        using PluginInfoVector = std::vector<PluginInfo>;
    }
}

#endif //COMMON_PLUGINREGISTRYIMPL_PLUGININFO_H
