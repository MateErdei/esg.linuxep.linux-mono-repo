/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_POLICYANDACTIONLISTENER_H
#define MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_POLICYANDACTIONLISTENER_H


#include <string>
#include <Common/DirectoryWatcher/IDirectoryWatcher.h>
#include "TaskDirectoryListener.h"

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class PolicyAndActionListener
        {
        public:
            /**
             * Watch <mcsPath>/policy and <mcsPath>/action for policies and actions
             * @param mcsPath
             */
            PolicyAndActionListener(
                    std::string mcsPath,
                    std::shared_ptr<Common::DirectoryWatcher::IDirectoryWatcher> directoryWatcher,
                    ITaskQueueSharedPtr taskQueue,
                    PluginCommunication::IPluginManager& pluginManager
                    );

            ~PolicyAndActionListener();
        private:
            TaskDirectoryListener m_policyListener;
            TaskDirectoryListener m_actionListener;
            std::shared_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
        };
    }
}


#endif //MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_POLICYANDACTIONLISTENER_H
