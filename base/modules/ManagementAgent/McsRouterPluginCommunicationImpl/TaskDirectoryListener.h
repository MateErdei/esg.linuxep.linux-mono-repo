/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_POLICYDIRECTORYLISTENER_H
#define EVEREST_BASE_POLICYDIRECTORYLISTENER_H


#include "Common/DirectoryWatcher/IDirectoryWatcher.h"

#include "TaskQueue.h"

#include <vector>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
       class TaskDirectoryListener: public Common::DirectoryWatcher::IDirectoryWatcherListener
        {
        public:
            TaskDirectoryListener(const std::string & path, std::shared_ptr<TaskQueue> taskQueue);
            ~TaskDirectoryListener() = default;
            
            std::string getPath() const override;
            void fileMoved(const std::string & filename) override;
            void watcherActive(bool active) override;

        private:
            std::string m_directoryPath;
            std::shared_ptr<TaskQueue> m_taskQueue;
            bool m_active;
        };

    }
}

#endif //EVEREST_BASE_POLICYDIRECTORYLISTENER_H
