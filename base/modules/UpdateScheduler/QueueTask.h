/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <condition_variable>
#include <mutex>
#include <list>

namespace UpdateScheduler
{
    struct Task
    {
        enum class TaskType{UpdateNow, Policy, Stop};
        TaskType  taskType;
        std::string Content;
    };

    class QueueTask {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;
    public:
        void push( Task );
        Task pop( );
        void pushStop();

    };

}


