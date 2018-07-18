/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <zconf.h>
#include <Common/FileSystem/IFileSystem.h>
#include <iostream>
#include "TaskQueueProcessor.h"
#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include "Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h"

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        TaskQueueProcessor::TaskQueueProcessor(ManagementAgent::PluginCommunication::IPluginManager& pluginManagerPtr, std::vector<std::string> directoriesToWatch)
        : m_pluginManagerPtr(pluginManagerPtr)
        , m_directoriesToWatch(directoriesToWatch)
        , m_running(false)
        {
            m_taskQueueProcessorState = TaskQueueProcessorState::Ready;
            m_taskQueue = std::make_shared<TaskQueue>();
            m_taskQueueProcessorThread = std::unique_ptr<TaskQueueProcessorThread>(new TaskQueueProcessorThread(pluginManagerPtr, m_taskQueue));
            init();
        }

        void TaskQueueProcessor::init()
        {
            // create a directory watcher and add a task listener for each of the directories to watch.

            m_directoryWatcher = std::unique_ptr<Common::DirectoryWatcherImpl::DirectoryWatcher>(
                    new Common::DirectoryWatcherImpl::DirectoryWatcher());

            for (auto &directory : m_directoriesToWatch)
            {
                TaskDirectoryListener directoryListener(directory, m_taskQueue);
                m_directoryListeners.push_back(directoryListener);
            }

            // ensure we pass a ref to the stored listeners.
            for (auto & listener : m_directoryListeners)
            {
                m_directoryWatcher->addListener(listener);
            }
        }
        
        void TaskQueueProcessor::start()
        {
            if( TaskQueueProcessorState::Ready )
            {
                m_taskQueueProcessorState = TaskQueueProcessorState::Started;
                m_directoryWatcher->startWatch();
                m_taskQueueProcessorThread->start();

            }
        }

        void TaskQueueProcessor::stop()
        {
            if ( m_taskQueueProcessorState == TaskQueueProcessorState::Stopped)
            {
                return;
            }

            m_taskQueueProcessorState = TaskQueueProcessorState::Stopped;
            
            // create stop task to unblock task list and stop queue thread.
            Task stopTask;
            stopTask.taskType = TaskType::Stop;
            stopTask.filePath = "Stop";

            m_taskQueue->push(stopTask);

            m_taskQueueProcessorThread->requestStop();

        }

        void TaskQueueProcessor::join()
        {
            m_taskQueueProcessorState = TaskQueueProcessor::Ready;
            m_taskQueueProcessorThread->join();
        }

    }

}