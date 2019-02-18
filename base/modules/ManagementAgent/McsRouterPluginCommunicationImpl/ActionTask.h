/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITask.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>

#include <string>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class ActionTask : public virtual Common::TaskQueue::ITask
        {
        public:
            ActionTask(PluginCommunication::IPluginManager& pluginManager, const std::string& filePath);
            void run() override;

        private:
            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_filePath;
        };
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
