// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <chrono>
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
            Stop
        };
        TaskType taskType;
        std::string Content;
    };

    class TaskQueue
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(Task);
        bool pop(Task&, int timeout);
        bool pop(Task&, std::chrono::milliseconds timeout);
        void pushStop();
    };

} // namespace Plugin
