/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PluginInfo.h"
#include "PluginInfo.pb.h"
#include "PluginRegistryException.h"
#include "Common/FileSystem/IFileSystem.h"
#include <google/protobuf/util/json_util.h>

namespace Common
{
    namespace PluginRegistryImpl
    {

        std::vector<std::string> PluginInfo::getAppIds() const
        {
            return m_appIds;
        }

        std::string PluginInfo::getPluginName() const
        {
            return m_pluginName;
        }

        std::string PluginInfo::getPluginIpcAddress() const
        {
            //FIXME: use the calculated plugin address from the plugin name
            return  "ipc:///tmp/" + m_pluginName + ".ipc";
        }

        std::string PluginInfo::getXmlTranslatorPath() const
        {
            return m_xmlTranslatorPath;
        }

        std::vector<std::string> PluginInfo::getExecutableArguments() const
        {
            return m_executableArguments;
        }

        PluginInfo::EnvPairs PluginInfo::getExecutableEnvironmentVariables() const
        {
            return m_executableEnvironmentVariables;
        }

        std::string PluginInfo::getExecutableFullPath() const
        {
            return m_executableFullPath;
        }

        void PluginInfo::setAppIds(const std::vector<std::string> &appIDs)
        {
            m_appIds = appIDs;
        }

        void PluginInfo::addAppIds(const std::string &appID)
        {
            m_appIds.push_back(appID);
        }

        void PluginInfo::setPluginName(const std::string &pluginName)
        {
            m_pluginName = pluginName;
        }

        void PluginInfo::setXmlTranslatorPath(const std::string &xmlTranslationPath)
        {
            m_xmlTranslatorPath = xmlTranslationPath;
        }

        void PluginInfo::setExecutableFullPath(const std::string &executableFullPath)
        {
            m_executableFullPath = executableFullPath;
        }

        void PluginInfo::setExecutableArguments(const std::vector<std::string> &executableArguments)
        {
            m_executableArguments = executableArguments;
        }

        void PluginInfo::addExecutableArguments(const std::string &executableArgument)
        {
            m_executableArguments.push_back(executableArgument);
        }

        void PluginInfo::setExecutableEnvironmentVariables(const PluginInfo::EnvPairs  &executableEnvironmentVariables)
        {
            m_executableEnvironmentVariables = executableEnvironmentVariables;
        }

        void PluginInfo::addExecutableEnvironmentVariables(const std::string& environmentName, const std::string &environmentValue)
        {
            m_executableEnvironmentVariables.emplace_back(environmentName, environmentValue);
        }

        PluginInfo PluginInfo::deserializeFromString(const std::string & settingsString)
        {
            using ProtoPluginInfo =  PluginInfoProto::PluginInfo;
            using namespace google::protobuf::util;
            ProtoPluginInfo protoPluginInfo;

            auto status = JsonStringToMessage(settingsString, &protoPluginInfo );
            if ( !status.ok())
            {

                  throw PluginRegistryException(std::string("Failed to load json string: ") + status.ToString());

            }
            PluginInfo pluginInfo;
            for ( auto & appid : protoPluginInfo.appids())
            {
                pluginInfo.addAppIds(appid);
            }

            pluginInfo.setPluginName(protoPluginInfo.pluginname());
            pluginInfo.setXmlTranslatorPath(protoPluginInfo.xmltranslatorpath());
            pluginInfo.setExecutableFullPath(protoPluginInfo.executablefullpath());

            for( auto & argv : protoPluginInfo.executablearguments())
            {
                pluginInfo.addExecutableArguments(argv);
            }

            for( auto & argEnv: protoPluginInfo.environmentvariables())
            {
                pluginInfo.addExecutableEnvironmentVariables(argEnv.name(), argEnv.value());
            }

            return pluginInfo;
        }

        std::vector<PluginInfo> PluginInfo::loadFromDirectoryPath(const std::string & directoryPath)
        {
            //TODO
            return std::vector<PluginInfo>();
        }
    }
}