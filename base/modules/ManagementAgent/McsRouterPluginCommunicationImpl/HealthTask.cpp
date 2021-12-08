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
    namespace McsRouterPluginCommunicationImpl
    {
        HealthTask::HealthTask(PluginCommunication::IPluginManager& pluginManager,
                               std::shared_ptr<HealthStatus> healthStatus)
            : m_pluginManager(pluginManager)
            , m_healthStatus(healthStatus)
        {
        }

        void HealthTask::run()
        {
            LOGDEBUG("Process health task");

            m_healthStatus->resetPluginHealthLists();

            for (const auto& pluginName : m_pluginManager.getRegisteredPluginNames())
            {
                ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthStatus =
                    m_pluginManager.getHealthStatusForPlugin(pluginName);

                m_healthStatus->addPluginHealth(pluginName, pluginHealthStatus);
            }

            // Hard coding MCSRouter status to Good
            ManagementAgent::PluginCommunication::PluginHealthStatus mcsRouterHealthStatus;
            mcsRouterHealthStatus.healthValue = 0;
            mcsRouterHealthStatus.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE_AND_THREAT;
            mcsRouterHealthStatus.displayName = "Sophos MCS Client";
            m_healthStatus->addPluginHealth("MCS", mcsRouterHealthStatus);

            std::pair<bool, std::string> statusXmlResult =  m_healthStatus->generateHealthStatusXml();

            Path tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            Path statusDir = Common::ApplicationConfiguration::applicationPathManager().getMcsStatusFilePath();
            std::string statusFilePath = Common::FileSystem::join(statusDir,"SHS_status.xml");

            // If this is the first run after the MA process starts up, statusXmlResult should always report true.
            if (statusXmlResult.first)
            {
                try
                {
                    Common::FileSystem::fileSystem()->writeFileAtomically(statusFilePath, statusXmlResult.second, tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
                }
                catch(const Common::FileSystem::IFileSystemException& ex)
                {
                    LOGWARN("Failed to write SHS status file with error, " << ex.what());
                    // make sure that write the status file is attempted again on next run.
                    m_healthStatus->resetCachedHealthStatusXml();
                }
            }

        }
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
