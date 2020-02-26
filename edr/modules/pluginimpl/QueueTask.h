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
            STOP,
            Policy,
            Query
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
        bool pop(Task&, int timeout);
        void pushStop();
        void pushOsqueryProcessDelayRestart();
        void pushOsqueryProcessFinished();
        void pushRestartOsquery();
        void pushPolicy(std::string policyXMl);
    };

} // namespace Plugin
