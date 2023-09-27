/******************************************************************************************************

Copyright 2018-2020
******************************************************************************************************/

#pragma once

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

namespace Plugin
{
    struct Task
    {
        enum class TaskType
        {
            Policy,
            Response,
            Stop
        };
        TaskType m_taskType;
        std::string m_content;
        std::string m_correlationId="";
    };

    class QueueTask
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(Task);
        Task pop();
        void pushStop();
    };

} // namespace Plugin
