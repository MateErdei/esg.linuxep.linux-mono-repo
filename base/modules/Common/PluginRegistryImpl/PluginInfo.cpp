/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PluginInfo.h"
#include "PluginInfo.pb.h"
#include "PluginRegistryException.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/MessageUtility.h"

#include <google/protobuf/util/json_util.h>

namespace Common
{
    namespace PluginRegistryImpl
    {

        std::vector<std::string> PluginInfo::getPolicyAppIds() const
        {
            return m_policyAppIds;
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

        void PluginInfo::setPolicyAppIds(const std::vector<std::string> &appIDs)
        {
            m_policyAppIds = appIDs;
        }

        void PluginInfo::addPolicyAppIds(const std::string &appID)
        {
            m_policyAppIds.push_back(appID);
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

        std::string PluginInfo::serializeToString(const PluginInfo & pluginInfo)
        {
            using ProtoPluginInfo =  PluginInfoProto::PluginInfo;
            ProtoPluginInfo pluginInfoProto;

            for(auto & policyAppId : pluginInfo.getPolicyAppIds())
            {
                pluginInfoProto.add_policyappids(policyAppId);
            }

            for(auto & statusAppId : pluginInfo.getStatusAppIds())
            {
                pluginInfoProto.add_statusappids(statusAppId);
            }

            pluginInfoProto.set_pluginname(pluginInfo.getPluginName());
            pluginInfoProto.set_xmltranslatorpath(pluginInfo.getXmlTranslatorPath());
            pluginInfoProto.set_executablefullpath(pluginInfo.getExecutableFullPath());

            for(auto & arg : pluginInfo.getExecutableArguments())
            {
                pluginInfoProto.add_executablearguments(arg);
            }

            PluginInfoProto::PluginInfo_EnvironmentVariablesT * environmentVariables = pluginInfoProto.add_environmentvariables();

            for(auto & envVar: pluginInfo.getExecutableEnvironmentVariables())
            {
                environmentVariables->set_name(envVar.first);
                environmentVariables->set_value(envVar.second);
            }

            return  Common::UtilityImpl::MessageUtility::protoBuf2Json(pluginInfoProto);
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

            for ( auto & statusAppid : protoPluginInfo.statusappids())
            {
                pluginInfo.addStatusAppIds(statusAppid);
            }

            for ( auto & policyAppid : protoPluginInfo.policyappids())
            {
                pluginInfo.addPolicyAppIds(policyAppid);
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
            std::vector<std::string> files = FileSystem::fileSystem()->listFiles(directoryPath);

            std::vector<PluginInfo> pluginInfoList;

            for(auto &pluginInfoFile : files)
            {
                if(pluginInfoFile.find(".json") != std::string::npos)
                {
                    try
                    {
                        std::string fileContent = FileSystem::fileSystem()->readFile(pluginInfoFile);
                        pluginInfoList.push_back(deserializeFromString(fileContent));
                    }
                    catch(PluginRegistryException &)
                    {
                        continue;
                    }
                    catch(Common::FileSystem::IFileSystemException &)
                    {
                        continue;
                    }
                }
            }

            if(pluginInfoList.empty())
            {
                throw PluginRegistryException("Failed to load any plugin registry information from: " + directoryPath);
            }

            return pluginInfoList;
        }

        std::vector<PluginInfo> PluginInfo::loadFromPluginRegistry()
        {
            return loadFromDirectoryPath(""); //Fixme get path from path manager class.
        }

        std::vector<std::string> PluginInfo::getStatusAppIds() const
        {
            return m_statusAppIds;
        }

        void PluginInfo::setStatusAppIds(const std::vector<std::string> &appIDs)
        {
            m_statusAppIds = appIDs;
        }

        void PluginInfo::addStatusAppIds(const std::string &appID)
        {
            m_statusAppIds.push_back(appID);
        }
    }
}