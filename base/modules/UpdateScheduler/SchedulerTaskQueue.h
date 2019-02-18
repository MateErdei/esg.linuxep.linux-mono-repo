/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

namespace UpdateScheduler
{
    struct SchedulerTask
    {
        enum class TaskType
        {
            UpdateNow,
            ScheduledUpdate,
            Policy,
            Stop,
            ShutdownReceived,
            SulDownloaderFinished,
            SulDownloaderFailedToStart,
            SulDownloaderTimedOut,
            SulDownloaderMonitorDetached
        };

        TaskType taskType;
        std::string content;
    };

    class SchedulerTaskQueue
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<SchedulerTask> m_list;

    public:
        void push(SchedulerTask);
        SchedulerTask pop();
        void pushStop();
    };

} // namespace UpdateScheduler
