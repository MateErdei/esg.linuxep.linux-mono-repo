/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_PLUGINCOMMUNICATIONQUEUEPROCESSOR_H
#define EVEREST_BASE_PLUGINCOMMUNICATIONQUEUEPROCESSOR_H

#include "../PluginCommunicationImpl/PluginManager.h"

#include "TaskQueue.h"
#include "TaskDirectoryListener.h"
#include "TaskQueueProcessorThread.h"

#include <memory>
#include <mutex>
#include <string>


namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {

        class TaskQueueProcessor
        {
        public:
            TaskQueueProcessor(ManagementAgent::PluginCommunication::IPluginManager& pluginManagerPtr, std::vector<std::string> directoriesToWatch);
            void init();
            void start();
            void stop();
            void join();            
        private:
             
            ManagementAgent::PluginCommunication::IPluginManager& m_pluginManagerPtr;
            std::vector<std::string> m_directoriesToWatch;
            bool m_running;
            enum TaskQueueProcessorState {Stopped, Started, Ready};
            TaskQueueProcessorState m_taskQueueProcessorState;
            std::unique_ptr<TaskQueueProcessorThread> m_taskQueueProcessorThread;
            std::shared_ptr<TaskQueue> m_taskQueue;
            std::vector<TaskDirectoryListener> m_directoryListeners;
            std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
        };
    }
}


#endif //EVEREST_BASE_PLUGINCOMMUNICATIONQUEUEPROCESSOR_H
