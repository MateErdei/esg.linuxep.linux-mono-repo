/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PluginInfo.h"
#include "PluginInfo.pb.h"
#include "PluginRegistryException.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/MessageUtility.h"
#include "Logger.h"
#include <google/protobuf/util/json_util.h>
#include <Common/Datatypes/StringUtils.h>

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
            return Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(m_pluginName);
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


            for(auto & envVar: pluginInfo.getExecutableEnvironmentVariables())
            {
                PluginInfoProto::PluginInfo_EnvironmentVariablesT * environmentVariables = pluginInfoProto.add_environmentvariables();
                environmentVariables->set_name(envVar.first);
                environmentVariables->set_value(envVar.second);
            }

            return  Common::UtilityImpl::MessageUtility::protoBuf2Json(pluginInfoProto);
        }

        PluginInfo PluginInfo::deserializeFromString(const std::string& settingsString, const std::string& pluginNameFromFilename)
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

            for ( const auto & statusAppid : protoPluginInfo.statusappids())
            {
                pluginInfo.addStatusAppIds(statusAppid);
            }

            for ( const auto & policyAppid : protoPluginInfo.policyappids())
            {
                pluginInfo.addPolicyAppIds(policyAppid);
            }

            std::string pluginname = protoPluginInfo.pluginname();
            if (pluginname.empty())
            {
                pluginname = pluginNameFromFilename;
            }
            else if (pluginNameFromFilename.empty())
            {
                // Pass
            }
            else if (pluginname != pluginNameFromFilename)
            {
                throw PluginRegistryException(
                        std::string("Inconsistent plugin name plugin_name_from_file=") + pluginNameFromFilename + " plugin_name_from_json=" + pluginname
                        );
            }
            pluginInfo.setPluginName(pluginname);
            pluginInfo.setXmlTranslatorPath(protoPluginInfo.xmltranslatorpath());
            pluginInfo.setExecutableFullPath(protoPluginInfo.executablefullpath());

            for( const auto & argv : protoPluginInfo.executablearguments())
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
            pluginInfoList.reserve(files.size());

            for(auto &pluginInfoFile : files)
            {
                if (Common::Datatypes::StringUtils::endswith(pluginInfoFile,".json"))
                {
                    std::string pluginName(extractPluginNameFromFilename(pluginInfoFile));
                    try
                    {
                        std::string fileContent = FileSystem::fileSystem()->readFile(pluginInfoFile);
                        pluginInfoList.emplace_back(deserializeFromString(fileContent, pluginName));
                    }
                    catch(PluginRegistryException &)
                    {
                        LOGWARN("Failed to load plugin information from file: " << pluginInfoFile);
                        continue;
                    }
                    catch(Common::FileSystem::IFileSystemException &)
                    {
                        LOGWARN("IO error, failed to obtain plugin information from file: " << pluginInfoFile);
                        continue;
                    }
                }
            }

            if(pluginInfoList.empty())
            {
                // There may be instances when there are no plugins available. Such as initial install.
                LOGWARN("Failed to load any plugin registry information from: " + directoryPath);
            }

            return pluginInfoList;
        }

        std::vector<PluginInfo> PluginInfo::loadFromPluginRegistry()
        {
            return loadFromDirectoryPath(Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath());
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

        std::pair<PluginInfo, bool> PluginInfo::loadPluginInfoFromRegistry(const std::string& pluginName)
        {
            auto tolowerstring = [](std::string& st) { std::transform(st.begin(), st.end(), st.begin(), ::tolower); };

            std::string lowercasepluginname = pluginName;
            tolowerstring(lowercasepluginname);

            std::string pluginFilePath;
            for (const std::string& filepath : Common::FileSystem::fileSystem()->listFiles(
                    Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath()))
            {
                std::string basename = Common::FileSystem::basename(filepath);

                tolowerstring(basename);
                if (basename.find(lowercasepluginname) != std::string::npos)
                {
                    pluginFilePath = filepath;
                }
            }

            if (pluginFilePath.empty())
            {
                return std::pair<PluginInfo, bool>(PluginInfo(), false);
            }
            try
            {
                std::string fileContent = FileSystem::fileSystem()->readFile(pluginFilePath);
                return std::pair<PluginInfo, bool>(deserializeFromString(fileContent, lowercasepluginname), true);

            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to load plugin info: " << ex.what());
            }
            return std::pair<PluginInfo, bool>(PluginInfo(), false);
        }

        std::string PluginInfo::extractPluginNameFromFilename(const std::string& filepath)
        {
            std::string basename = Common::FileSystem::basename(filepath);
            std::string prefix = basename.substr(0, basename.size() - 5);
            return prefix;
        }

    }
}