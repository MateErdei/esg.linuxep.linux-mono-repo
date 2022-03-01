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

    class QueueTask
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;

    public:
        void push(Task);
        Task pop();
        bool pop(Task&, int timeout);
        void pushStop();
        bool empty();
        bool queueContainsPolicyTask();
    };

} // namespace Plugin
