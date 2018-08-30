/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <Common/TaskQueue/ITask.h>
#include <string>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class PolicyTask
            : public virtual Common::TaskQueue::ITask
        {
        public:
            static void distributePolicy(  PluginCommunication::IPluginManager& m_pluginManager, const std::string & filepath);

            void run() override;
            PolicyTask(
                    PluginCommunication::IPluginManager& pluginManager,
                    std::string filePath
                    );

        private:

            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_filePath;
        };
    }
}



