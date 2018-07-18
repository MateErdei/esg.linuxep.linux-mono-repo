/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFileSystem.h"
#include "TaskQueueProcessorThread.h"
#include "Logger.h"

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {

        TaskQueueProcessorThread::TaskQueueProcessorThread(PluginCommunication::IPluginManager &pluginManagerPtr, std::shared_ptr<TaskQueue> taskQueue)
        : m_pluginManagerPtr(pluginManagerPtr)
        , m_taskQueue(taskQueue)
        , m_isRunning(false)
        {
        }

        TaskQueueProcessorThread::~TaskQueueProcessorThread()
        {
        }

        Task TaskQueueProcessorThread::getNextTask()
        {
            Task task = m_taskQueue->pop();

            if(task.taskType == TaskType::Stop)
            {
                return task;
            }

            // Get appid and task type from file name.
            try
            {
                task.payload = Common::FileSystem::fileSystem()->readFile(task.filePath);
            }
            catch(Common::FileSystem::IFileSystemException& e)
            {
                LOGWARN("Failed to read from file: " << task.filePath);
            }

            std::string basename = Common::FileSystem::fileSystem()->basename(task.filePath);

            size_t pos = basename.find("-");

            if (pos != std::string::npos)
            {
                task.appId = basename.substr(0, pos);
            }

            if (basename.find("policy") != std::string::npos)
            {
                task.taskType = TaskType::Policy;
            } else
            {
                task.taskType = TaskType::Unknown;
            }

            return task;
        }

        void TaskQueueProcessorThread::run()
        {
            m_isRunning = true;

            announceThreadStarted();

            while ( !stopRequested() && m_isRunning)
            {
                Task task = getNextTask();

                switch (task.taskType)
                {
                    case TaskType::Policy:
                        m_pluginManagerPtr.applyNewPolicy(task.appId, task.payload);
                        LOGDEBUG("Applying new policy from: " << task.filePath);
                        continue;
                    case TaskType::Stop:
                        LOGDEBUG("Received stop request, stopping task queue processing thread.");
                        m_isRunning = false;
                        continue;
                    default:
                        LOGWARN("Unknown task found in task queue");
                        continue;
                }
            }
        }




    }
}