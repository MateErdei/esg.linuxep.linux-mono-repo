/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "HealthTask.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <ManagementAgent/LoggerImpl/Logger.h>

#include <sys/stat.h>

namespace ManagementAgent
{
    namespace HealthStatusImpl
    {
        HealthTask::HealthTask(PluginCommunication::IPluginManager& pluginManager)
            : m_pluginManager(pluginManager)
        {
        }

        void HealthTask::run()
        {
            LOGDEBUG("Process health task");

            std::map<std::string, PluginCommunication::PluginHealthStatus> prevServiceHealth =
                m_pluginManager.getSharedHealthStatusObj()->getPluginServiceHealthLists();
            std::map<std::string, PluginCommunication::PluginHealthStatus> prevThreatServiceHealth =
                m_pluginManager.getSharedHealthStatusObj()->getPluginThreatServiceHealthLists();

            m_pluginManager.getSharedHealthStatusObj()->resetPluginHealthLists();

            for (const auto& pluginName : m_pluginManager.getRegisteredPluginNames())
            {
                bool prevHealthMissing = false;
                PluginCommunication::PluginHealthStatus prevPluginServiceHealth;
                PluginCommunication::PluginHealthStatus prevPluginThreatServiceHealth;

                if (prevServiceHealth.count(pluginName) != 0)
                {
                    prevPluginServiceHealth = prevServiceHealth[pluginName];
                }
                if (prevThreatServiceHealth.count(pluginName) != 0)
                {
                    prevPluginThreatServiceHealth = prevThreatServiceHealth[pluginName];
                }

                if (prevPluginServiceHealth.healthValue == 2 || prevPluginThreatServiceHealth.healthValue == 2)
                {
                    prevHealthMissing = true;
                }

                ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthStatus =
                    m_pluginManager.getHealthStatusForPlugin(pluginName, prevHealthMissing);

                m_pluginManager.getSharedHealthStatusObj()->addPluginHealth(pluginName, pluginHealthStatus);
            }

            // Hard coding MCSRouter status to Good
            ManagementAgent::PluginCommunication::PluginHealthStatus mcsRouterHealthStatus;
            mcsRouterHealthStatus.healthValue = 0;
            mcsRouterHealthStatus.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE_AND_THREAT;
            mcsRouterHealthStatus.displayName = "Sophos MCS Client";
            m_pluginManager.getSharedHealthStatusObj()->addPluginHealth("MCS", mcsRouterHealthStatus);

            std::pair<bool, std::string> statusXmlResult =  m_pluginManager.getSharedHealthStatusObj()->generateHealthStatusXml();

            Path tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            Path statusDir = Common::ApplicationConfiguration::applicationPathManager().getMcsStatusFilePath();
            std::string statusFilePath = Common::FileSystem::join(statusDir,"SHS_status.xml");

            // If this is the first run after the MA process starts up, statusXmlResult should always report true.
            if (statusXmlResult.first)
            {
                try
                {
                    LOGINFO("Writing health status");
                    Common::FileSystem::fileSystem()->writeFileAtomically(statusFilePath, statusXmlResult.second, tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
                }
                catch(const Common::FileSystem::IFileSystemException& ex)
                {
                    LOGWARN("Failed to write SHS status file with error, " << ex.what());
                    // make sure that write the status file is attempted again on next run.
                    m_pluginManager.getSharedHealthStatusObj()->resetCachedHealthStatusXml();
                }
            }

        }
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
