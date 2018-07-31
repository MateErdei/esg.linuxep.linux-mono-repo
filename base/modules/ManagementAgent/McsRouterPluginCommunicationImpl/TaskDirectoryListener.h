/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include <ManagementAgent/PluginCommunication/IPluginManager.h>

#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include <Common/TaskQueue/ITaskQueue.h>

#include <vector>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        using ITaskQueueSharedPtr = std::shared_ptr<Common::TaskQueue::ITaskQueue>;

        class TaskDirectoryListener: public virtual Common::DirectoryWatcher::IDirectoryWatcherListener
        {
        public:
            TaskDirectoryListener(
                    const std::string & path,
                    ITaskQueueSharedPtr taskQueue,
                    PluginCommunication::IPluginManager& pluginManager
                );
            ~TaskDirectoryListener() = default;
            
            std::string getPath() const override;
            void fileMoved(const std::string & filename) override;
            void watcherActive(bool active) override;

        private:
            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_directoryPath;
            ITaskQueueSharedPtr m_taskQueue;
            bool m_active;
        };

    }
}


