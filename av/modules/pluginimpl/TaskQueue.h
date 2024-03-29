// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/StoppableSleeper.h"

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
            Stop,
            Action,
            ScanComplete,
            ThreatDetected,
            SendCleanEvent,
            SendStatus,
            SendRestoreEvent
        };
        TaskType taskType;
        std::string Content;
        std::string appId = "";
    };

    class TaskQueue : public common::StoppableSleeper
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;
        bool m_stopQueued = false;

    public:
        using clock_t = std::chrono::steady_clock;
        using time_point_t = std::chrono::time_point<clock_t>;

        TaskQueue() = default;
        TaskQueue(const TaskQueue&) = delete;

        void push(Task);
        Task pop();
        bool pop(Task& task, const time_point_t& timeout_time);
        void pushStop();
        bool empty();
        bool queueContainsPolicyTask();

        /**
         *
         * @param sleepTime
         * @return True if the sleep was stopped
         */
        bool stoppableSleep(duration_t sleepTime) override;
    };

} // namespace Plugin
