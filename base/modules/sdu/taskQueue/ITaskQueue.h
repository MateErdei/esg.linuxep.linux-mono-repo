// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace RemoteDiagnoseImpl
{
    struct Task
    {
        enum class TaskType
        {
            ACTION,
            STOP,
            DIAGNOSE_MONITOR_DETACHED,
            DIAGNOSE_TIMED_OUT,
            DIAGNOSE_FINISHED,
            DIAGNOSE_FAILED_TO_START,
            UNDEFINED,
        };
        TaskType taskType;
        std::string Content;
    };
    class ITaskQueue
    {
    public:
        virtual ~ITaskQueue() = default;
        virtual void push(Task task) = 0;
        virtual void pushPriority(Task task) = 0;
        virtual Task pop() = 0;
        virtual Task pop(bool isRunning) = 0;
    };

} // namespace RemoteDiagnoseImpl
