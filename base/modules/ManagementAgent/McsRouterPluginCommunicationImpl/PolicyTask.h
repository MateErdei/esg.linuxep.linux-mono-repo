/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_POLICYTASK_H
#define MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_POLICYTASK_H


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


#endif //MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_POLICYTASK_H
