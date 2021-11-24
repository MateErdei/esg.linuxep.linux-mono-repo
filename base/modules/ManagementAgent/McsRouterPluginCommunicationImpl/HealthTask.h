/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "HealthStatus.h"
#include <Common/TaskQueue/ITask.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class HealthTask : public Common::TaskQueue::ITask
        {
        public:
            HealthTask(PluginCommunication::IPluginManager& pluginManager, std::shared_ptr<HealthStatus> healthStatus);

            void run() override;

        private:
            PluginCommunication::IPluginManager& m_pluginManager;
            std::shared_ptr<HealthStatus> m_healthStatus;
        };
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
