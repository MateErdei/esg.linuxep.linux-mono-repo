/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

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
        enum class TaskType : int
        {
            START_OSQUERY,
            OSQUERY_PROCESS_FINISHED,
            OSQUERY_PROCESS_FAILED_TO_START,
            STOP,
            POLICY,
            QUERY,
            QUEUE_OSQUERY_RESTART
        };
        TaskType m_taskType;
        std::string m_content;
        std::string m_correlationId="";
        std::string m_appId="";
    };

    class QueueTask
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(const Task&);
        void pushIfNotAlreadyInQueue(Task);
        bool pop(Task&, int timeout);
        void pushStop();
        void pushOsqueryProcessDelayRestart();
        void pushOsqueryProcessFinished();
        void pushStartOsquery();
        void pushPolicy(const std::string& appId, const std::string& policyXMl);
        void pushOsqueryRestart(const std::string& reason);
        void clearQueue();
    };

} // namespace Plugin
