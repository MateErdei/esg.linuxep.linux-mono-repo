/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_TASKQUEUEPROCESSORTHREAD_H
#define EVEREST_BASE_TASKQUEUEPROCESSORTHREAD_H


#include "Common/Threads/AbstractThread.h"
#include "ManagementAgent/PluginCommunication/IPluginManager.h"
#include "TaskQueue.h"

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {

        class TaskQueueProcessorThread : public Common::Threads::AbstractThread
        {
        public:
            TaskQueueProcessorThread(PluginCommunication::IPluginManager& pluginManagerPtr, std::shared_ptr<TaskQueue> taskQueue);

            ~TaskQueueProcessorThread();

        private:
            void run() override;

            Task getNextTask();

            PluginCommunication::IPluginManager& m_pluginManagerPtr;
            std::shared_ptr<TaskQueue> m_taskQueue;
            bool m_isRunning;
        };
    }
}

#endif //EVEREST_BASE_TASKQUEUEPROCESSORTHREAD_H