/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_TASKQUEUE_H
#define EVEREST_BASE_TASKQUEUE_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <list>
#include <queue>


namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {

        enum TaskType { Unknown, Policy, Action, Stop };

        struct Task
        {
            TaskType taskType;
            std::string filePath;
            std::string appId;
            std::string payload;
        };

        class TaskQueue
        {
        public:
            TaskQueue();
            ~TaskQueue();
            void push(Task task);
            Task pop();

        private:
            std::unique_ptr<std::mutex> m_queueMutex;
            std::unique_ptr<std::condition_variable> m_condition;
            std::list<Task> m_tasks;
            bool process;
        };

    }
}
#endif //EVEREST_BASE_TASKQUEUE_H
