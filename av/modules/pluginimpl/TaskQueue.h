// Copyright 2018-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "modules/common/StoppableSleeper.h"

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
            SendStatus
        };
        TaskType taskType;
        std::string Content;
        std::string appId="";

        [[nodiscard]] std::string getTaskName() const {
            switch (taskType)
            {
                case TaskType::Policy:
                    return "Policy";
                case TaskType::Stop:
                    return "Stop";
                case TaskType::Action:
                    return "Action";
                case TaskType::ScanComplete:
                    return "ScanComplete";
                case TaskType::ThreatDetected:
                    return "ThreatDetected";
                case TaskType::SendStatus:
                    return "SendStatus";
            }

            return "Unknown task type";
        }
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
