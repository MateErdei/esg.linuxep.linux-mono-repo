/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

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
            RESTARTOSQUERY,
            OSQUERYPROCESSFINISHED,
            OSQUERYPROCESSFAILEDTOSTART,
            STOP
        };
        TaskType taskType;
        std::string Content;
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
        void pushOsqueryProcessDelayRestart();
        void pushOsqueryProcessFinished();
        void pushRestartOsquery();
    };

} // namespace Plugin
