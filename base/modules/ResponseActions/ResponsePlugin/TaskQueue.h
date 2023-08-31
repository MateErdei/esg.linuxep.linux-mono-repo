// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

namespace ResponsePlugin
{
    struct Task
    {
        enum class TaskType
        {
            ACTION, STOP, CHECK_QUEUE };
        TaskType m_taskType;
        std::string m_content;
        std::string m_appId = "";
        std::string m_correlationId = "";
    };

    class TaskQueue
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(const Task&);
        Task pop();
        void pushStop();
        bool pop(Task&, int timeout);
    };

} // namespace ResponsePlugin
