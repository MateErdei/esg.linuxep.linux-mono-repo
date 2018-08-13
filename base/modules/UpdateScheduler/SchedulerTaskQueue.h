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
    struct SchedulerTask
    {
        enum class TaskType{UpdateNow, ScheduledUpdate, Policy, Stop, SulDownloaderFinished, SulDownloaderFailedToStart};
        TaskType  taskType;
        std::string Content;
    };

    class SchedulerTaskQueue {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<SchedulerTask> m_list;
    public:
        void push( SchedulerTask );
        SchedulerTask pop( );
        void pushStop();

    };

}


