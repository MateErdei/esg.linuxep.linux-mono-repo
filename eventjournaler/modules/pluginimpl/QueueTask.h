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
        enum class TaskType
        {
            POLICY,
            EXAMPLETASK,
            STOP
        };
        TaskType m_taskType;
        std::string m_content;
        std::string m_appId="";
        std::string m_correlationId="";
    };

    class QueueTask
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(const Task&);
        Task pop();
        void pushStop();
        bool pop(Task&, int timeout);
        void pushPolicy(const std::string& appId, const std::string& policyXMl);
    };

} // namespace Plugin
