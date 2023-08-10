// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PluginInfo.h"

#include "Logger.h"
#include "Common/PluginRegistryImpl/PluginInfo.pb.h"
#include "PluginRegistryException.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/ProtobufUtil/MessageUtility.h"
#include "Common/UtilityImpl/StringUtils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <google/protobuf/util/json_util.h>

#pragma GCC diagnostic pop

namespace Common
{
    namespace PluginRegistryImpl
    {
        std::vector<std::string> PluginInfo::getPolicyAppIds() const { return m_policyAppIds; }

        std::vector<std::string> PluginInfo::getActionAppIds() const { return m_actionAppIds; }

        std::string PluginInfo::getPluginName() const { return m_pluginName; }

        std::string PluginInfo::getPluginIpcAddress() const
        {
            return Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress(m_pluginName);
        }

        std::string PluginInfo::getXmlTranslatorPath() const { return m_xmlTranslatorPath; }

        bool PluginInfo::getIsManagedPlugin() const { return m_isManagedPlugin; }

        bool PluginInfo::getHasThreatServiceHealth() const
        {
            return m_hasThreatServiceHealth;
        }

        void PluginInfo::setHasThreatServiceHealth(const bool hasThreatServiceHealth)
        {
            m_hasThreatServiceHealth = hasThreatServiceHealth;
        }

        bool PluginInfo::getHasServiceHealth() const
        {
            return m_hasServiceHealth;
        }

        void PluginInfo::setHasServiceHealth(const bool hasServiceHealth)
        {
            m_hasServiceHealth = hasServiceHealth;
        }

        std::string PluginInfo::getDisplayPluginName() const
        {
            return m_displayPluginName;
        }

        void PluginInfo::setDisplayPluginName(const std::string& displayPluginName)
        {
            m_displayPluginName = displayPluginName;
        }

        void PluginInfo::setPolicyAppIds(const std::vector<std::string>& appIDs) { m_policyAppIds = appIDs; }

        void PluginInfo::addPolicyAppIds(const std::string& appID) { m_policyAppIds.push_back(appID); }

        void PluginInfo::addActionAppIds(const std::string& appID) { m_actionAppIds.push_back(appID); }

        void PluginInfo::setPluginName(const std::string& pluginName) { m_pluginName = pluginName; }

        void PluginInfo::setXmlTranslatorPath(const std::string& xmlTranslationPath)
        {
            m_xmlTranslatorPath = xmlTranslationPath;
        }
        void PluginInfo::setIsManagedPlugin(bool isManaged) { m_isManagedPlugin = isManaged; }

        std::string PluginInfo::serializeToString(const PluginInfo& pluginInfo)
        {
            using ProtoPluginInfo = PluginInfoProto::PluginInfo;
            ProtoPluginInfo pluginInfoProto;

            for (auto& policyAppId : pluginInfo.getPolicyAppIds())
            {
                pluginInfoProto.add_policyappids(policyAppId);
            }

            for (auto& statusAppId : pluginInfo.getStatusAppIds())
            {
                pluginInfoProto.add_statusappids(statusAppId);
            }

            if (pluginInfo.getIsManagedPlugin())
            {
                pluginInfoProto.set_pluginname(pluginInfo.getPluginName());
            }
            pluginInfoProto.set_xmltranslatorpath(pluginInfo.getXmlTranslatorPath());
            pluginInfoProto.set_executableuserandgroup(pluginInfo.getExecutableUserAndGroupAsString());
            pluginInfoProto.set_executablefullpath(pluginInfo.getExecutableFullPath());

            for (auto& arg : pluginInfo.getExecutableArguments())
            {
                pluginInfoProto.add_executablearguments(arg);
            }

            for (auto& envVar : pluginInfo.getExecutableEnvironmentVariables())
            {
                PluginInfoProto::PluginInfo_EnvironmentVariablesT* environmentVariables =
                    pluginInfoProto.add_environmentvariables();
                environmentVariables->set_name(envVar.first);
                environmentVariables->set_value(envVar.second);
            }
            pluginInfoProto.set_secondstoshutdown(pluginInfo.getSecondsToShutDown());
            pluginInfoProto.set_threatservicehealth(pluginInfo.getHasThreatServiceHealth());
            pluginInfoProto.set_servicehealth(pluginInfo.getHasServiceHealth());
            pluginInfoProto.set_displaypluginname(pluginInfo.getDisplayPluginName());

            return Common::ProtobufUtil::MessageUtility::protoBuf2Json(pluginInfoProto);
        }

        PluginInfo PluginInfo::deserializeFromString(
            const std::string& settingsString,
            const std::string& pluginNameFromFilename)
        {
            using ProtoPluginInfo = PluginInfoProto::PluginInfo;
            using namespace google::protobuf::util;
            ProtoPluginInfo protoPluginInfo;
            JsonParseOptions jsonParseOptions;
            jsonParseOptions.ignore_unknown_fields = true;

            auto status = JsonStringToMessage(settingsString, &protoPluginInfo, jsonParseOptions);

            if (!status.ok())
            {
                throw PluginRegistryException(std::string("Failed to load json string: ") + status.ToString());
            }

            PluginInfo pluginInfo;

            for (const auto& statusAppid : protoPluginInfo.statusappids())
            {
                pluginInfo.addStatusAppIds(statusAppid);
            }

            for (const auto& policyAppid : protoPluginInfo.policyappids())
            {
                pluginInfo.addPolicyAppIds(policyAppid);
            }

            for (const auto& actionAppid : protoPluginInfo.actionappids())
            {
                pluginInfo.addActionAppIds(actionAppid);
            }

            std::string pluginname = protoPluginInfo.pluginname();
            if (pluginname.empty())
            {
                // all plugins without names in the json configs are not managed by sophos_management
                // this covers mcsrouter and sophos_management itsself
                pluginInfo.setIsManagedPlugin(false);
                pluginname = pluginNameFromFilename;
            }
            else if (pluginname != pluginNameFromFilename)
            {
                throw PluginRegistryException(
                    std::string("Inconsistent plugin name plugin_name_from_file=") + pluginNameFromFilename +
                    " plugin_name_from_json=" + pluginname);
            }
            pluginInfo.setPluginName(pluginname);
            pluginInfo.setXmlTranslatorPath(protoPluginInfo.xmltranslatorpath());
            pluginInfo.setExecutableUserAndGroup(protoPluginInfo.executableuserandgroup());
            pluginInfo.setExecutableFullPath(protoPluginInfo.executablefullpath());
            pluginInfo.setSecondsToShutDown(protoPluginInfo.secondstoshutdown());

            for (const auto& argv : protoPluginInfo.executablearguments())
            {
                pluginInfo.addExecutableArguments(argv);
            }

            for (auto& argEnv : protoPluginInfo.environmentvariables())
            {
                pluginInfo.addExecutableEnvironmentVariables(argEnv.name(), argEnv.value());
            }

            pluginInfo.setHasThreatServiceHealth(protoPluginInfo.threatservicehealth());
            pluginInfo.setHasServiceHealth(protoPluginInfo.servicehealth());
            pluginInfo.setDisplayPluginName(protoPluginInfo.displaypluginname());

            return pluginInfo;
        }

        std::vector<PluginInfo> PluginInfo::loadFromDirectoryPath(const std::string& directoryPath)
        {
            std::vector<std::string> files = FileSystem::fileSystem()->listFiles(directoryPath);

            std::vector<PluginInfo> pluginInfoList;
            pluginInfoList.reserve(files.size());

            for (auto& pluginInfoFile : files)
            {
                if (Common::UtilityImpl::StringUtils::endswith(pluginInfoFile, ".json"))
                {
                    std::string pluginName(extractPluginNameFromFilename(pluginInfoFile));
                    try
                    {
                        std::string fileContent = FileSystem::fileSystem()->readFile(pluginInfoFile);
                        pluginInfoList.emplace_back(deserializeFromString(fileContent, pluginName));
                    }
                    catch (PluginRegistryException& ex)
                    {
                        LOGWARN("Failed to load plugin information from file: " << pluginInfoFile);
                        std::string reason = ex.what();
                        LOGSUPPORT(reason);
                        continue;
                    }
                    catch (Common::FileSystem::IFileSystemException&)
                    {
                        LOGWARN("IO error, failed to obtain plugin information from file: " << pluginInfoFile);
                        continue;
                    }
                }
            }

            if (pluginInfoList.empty())
            {
                // There may be instances when there are no plugins available. Such as initial install.
                LOGWARN("Failed to load any plugin registry information from: " + directoryPath);
            }

            return pluginInfoList;
        }

        std::vector<PluginInfo> PluginInfo::loadFromPluginRegistry()
        {
            return loadFromDirectoryPath(
                Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath());
        }

        std::vector<std::string> PluginInfo::getStatusAppIds() const { return m_statusAppIds; }

        void PluginInfo::setStatusAppIds(const std::vector<std::string>& appIDs) { m_statusAppIds = appIDs; }

        void PluginInfo::addStatusAppIds(const std::string& appID) { m_statusAppIds.push_back(appID); }

        std::pair<PluginInfo, bool> PluginInfo::loadPluginInfoFromRegistry(const std::string& pluginName)
        {
            Path pluginFilePath = Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getPluginRegistryPath(),
                pluginName + ".json");

            if (!Common::FileSystem::fileSystem()->isFile(pluginFilePath))
            {
                return std::pair<PluginInfo, bool>(PluginInfo(), false);
            }
            try
            {
                std::string fileContent = FileSystem::fileSystem()->readFile(pluginFilePath);
                return std::pair<PluginInfo, bool>(deserializeFromString(fileContent, pluginName), true);
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

    } // namespace PluginRegistryImpl
} // namespace Common