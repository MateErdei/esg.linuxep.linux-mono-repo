/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "HealthStatus.h"
#include <Common/TaskQueue/ITask.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>

namespace ManagementAgent
{
    namespace HealthStatusImpl
    {
        class HealthTask : public Common::TaskQueue::ITask
        {
        public:
            explicit HealthTask(PluginCommunication::IPluginManager& pluginManager);

            void run() override;

        private:
            PluginCommunication::IPluginManager& m_pluginManager;
        };
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
