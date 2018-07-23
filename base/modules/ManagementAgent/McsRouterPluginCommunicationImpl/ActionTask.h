/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_ACTIONTASK_H
#define MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_ACTIONTASK_H


#include <ManagementAgent/PluginCommunication/IPluginManager.h>

#include <Common/TaskQueue/ITask.h>

#include <string>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class ActionTask
            : public virtual Common::TaskQueue::ITask
        {
        public:
            ActionTask(PluginCommunication::IPluginManager& pluginManager, const std::string& filePath);
            void run() override;
        private:

            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_filePath;
        };
    }
}


#endif //MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_ACTIONTASK_H
