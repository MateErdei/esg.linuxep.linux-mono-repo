/******************************************************************************************************

Copyright 2018-2022 Sophos Limited.  All rights reserved.

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
        std::string appId = "";
    };

    class SchedulerTaskQueue
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<SchedulerTask> m_list;

    public:
        void push(SchedulerTask);
        bool pop(SchedulerTask& task, int timeout);
        void pushStop();
        void pushFront(SchedulerTask task);
    };

} // namespace UpdateScheduler
