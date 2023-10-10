// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ProcessImpl/ProcessInfo.h"

#include <string>
#include <vector>

namespace Common::PluginRegistryImpl
{
    class PluginInfo;

    using PluginInfoVector = std::vector<PluginInfo>;

    class PluginInfo : public ProcessImpl::ProcessInfo
    {
    private:
        PluginInfo(const PluginInfo&) = default;
        PluginInfo& operator=(const PluginInfo&) = default;

    public:
        PluginInfo() = default;
        PluginInfo(PluginInfo&&) = default;
        PluginInfo& operator=(PluginInfo&&) = default;

        void copyFrom(const PluginInfo& other) { *this = other; }

        using EnvPairs = Common::Process::EnvPairVector;

        /**
         * Used to get stored policy appIds
         * @return list of app ids for the policies the plugin will support
         */
        std::vector<std::string> getPolicyAppIds() const;
        /**
         * Used to get stored action appIds
         * @return list of app ids for the actions the plugin will support
         */
        std::vector<std::string> getActionAppIds() const;
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
         * Used to get the flag that indicates if the plugin is managemed by sophos management agent
         * @return bool flag.
         */
        bool getIsManagedPlugin() const;

        /**
         * Used to store the given Policy AppIds the plugin is interested in.
         * @param list of appIDs
         */
        void setPolicyAppIds(const std::vector<std::string>& appIDs);

        /**
         * Used to add a single Policy appId to the stored PolicyAppId list.
         * @param appID
         */
        void addPolicyAppIds(const std::string& appID);

        /**
         * Used to add a single Action appId to the stored ActionAppId list.
         * @param appID
         */
        void addActionAppIds(const std::string& appID);

        /**
         * Used to store the given Status AppIds the plugin is interested in.
         * @param list of appIDs
         */
        void setStatusAppIds(const std::vector<std::string>& appIDs);

        /**
         * Used to add a single status appId to the stored StatusAppId list.
         * @param appID
         */
        void addStatusAppIds(const std::string& appID);

        /**
         * Used to store the given plugin name
         * @param pluginName
         */
        void setPluginName(const std::string& pluginName);

        /**
         * Used to store the given path for the location of the XML translator used by the plugin
         * @param xmlTranslationPath
         */
        void setXmlTranslatorPath(const std::string& xmlTranslationPath);

        /**
         * Used to store the deduced flag that indicates whether a plugin is managed by sophos_management
         * @param isManaged
         */
        void setIsManagedPlugin(bool isManaged);

        /**
         * Used to check if the plugin supports threat service health
         * @return true, if the plugin supports threat service health, false otherwise
         */
        bool getHasThreatServiceHealth() const;

        /**
         * Used to set the has threat service health if the plugin supports it
         * @param hasThreatServiceHealth
         */
        void setHasThreatServiceHealth(const bool hasThreatServiceHealth);

        /**
         * Used to check if the plugin supports service health
         * @return true, if the plugin supports service health, false otherwise
         */
        bool getHasServiceHealth() const;

        /**
         * Used to set the has service health if the plugin supports it
         * @param hasServiceHealth
         */
        void setHasServiceHealth(const bool hasServiceHealth);

        /**
         * Gets the name of the plugin used for display to customers
         * @return string show the display name given to a plugin.  Empty if not set.
         */
        std::string getDisplayPluginName() const;

        /**
         * Sets the name of the plugin used for display to customers
         * @param displayPluginName
         */
        void setDisplayPluginName(const std::string& displayPluginName);

        /**
         * Serialize pluginInfo object into protobuf message.
         * @param pluginInfo object to be serialized
         * @return pluginInfo represented as protobuf object.
         */
        static std::string serializeToString(const PluginInfo& pluginInfo);

        /**
         * Deserialize a Json containing the PluginInfo into the PluginInfo object.
         *
         * @throws PluginRegistryException if the json content can not be safely deserialized into the PluginInfo.
         *         It does not provide guarantee that the values for AppIds, ExecutablePaths, etc are correct to
         * use, The exception is thrown only to signal that the content of the json was malformed and could not be
         * parsed. It is the responsibility of the whoever used PluginInfo to verify that the content is 'Adequate'
         * to be used.
         *
         * @return PluginInfo parsed from the serializedPluginInfo.
         */
        static PluginInfo deserializeFromString(const std::string& serializedPluginInfo, const std::string& pluginName);

        /**
         * List the json entries from the directoryPath and load them into a vector of PluginInfo.
         * Failed to load specific entries will be 'silently' ignored. Although a log warning will be produced.
         *
         * @code
         * Uses deserializeFromString to convert json files into PluginInfo objects
         * @return A list of pluginInfo objects containing the loaded from the files read from disk.
         */
        static PluginInfoVector loadFromDirectoryPath(const std::string& directoryPath);

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

        /**
         * Load a new plugin from the registry by name.
         *
         * LoadSuccess: bool: True if we have loaded the info.
         *
         * @param pluginName
         * @return pair<PluginInfo, LoadSuccess>
         */
        static std::pair<PluginInfo, bool> loadPluginInfoFromRegistry(const std::string& pluginName);

        /**
         * From a filepath extract the non-extension part of the basename, which should be the plugin name
         *
         * @param filepath
         * @return
         */
        static std::string extractPluginNameFromFilename(const std::string& filepath);

    private:
        std::vector<std::string> m_policyAppIds;
        std::vector<std::string> m_actionAppIds;
        std::vector<std::string> m_statusAppIds;
        std::string m_pluginName;
        std::string m_xmlTranslatorPath;
        bool m_isManagedPlugin = true;
        bool m_hasThreatServiceHealth = false;
        bool m_hasServiceHealth = false;
        std::string m_displayPluginName;
    };

    using PluginInfoPtr = std::unique_ptr<PluginInfo>;
} // namespace Common::PluginRegistryImpl
