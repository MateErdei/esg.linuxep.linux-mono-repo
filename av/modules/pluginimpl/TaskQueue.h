/******************************************************************************************************

Copyright 2018-2022 Sophos Limited.  All rights reserved.

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

    class TaskQueue
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(Task);
        Task pop();
        bool pop(Task& task, const std::chrono::time_point<std::chrono::steady_clock>& timeout_time);
        void pushStop();
        bool empty();
        bool queueContainsPolicyTask();
    };

} // namespace Plugin
