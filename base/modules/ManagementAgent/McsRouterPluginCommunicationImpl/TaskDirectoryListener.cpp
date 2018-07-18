/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Common/FileSystem/IFileSystem.h"
#include "TaskDirectoryListener.h"

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        TaskDirectoryListener::TaskDirectoryListener(const std::string &directoryPath, std::shared_ptr<TaskQueue> taskQueue)
        : m_directoryPath(directoryPath)
        , m_taskQueue(taskQueue)
        , m_active(false)

        {
                
        }

        std::string TaskDirectoryListener::getPath() const
        {
            return m_directoryPath;
        }
        
        void TaskDirectoryListener::fileMoved(const std::string & filename)
        {
            Task task;
            std::string fullPath = Common::FileSystem::fileSystem()->join(getPath(), filename);

            task.filePath = fullPath;
            m_taskQueue->push(task);
        }
        

        void TaskDirectoryListener::watcherActive(bool active)
        {
            m_active = active;
        }
    }
}
