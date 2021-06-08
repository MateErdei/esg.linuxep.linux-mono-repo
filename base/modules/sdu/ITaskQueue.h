/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
            DiagnoseMonitorDetached,
            DiagnoseTimedOut,
            DiagnoseFinished,
            DiagnoseFailedToStart,
            Undefined,
        };
        TaskType taskType;
        std::string Content;
    };
    class ITaskQueue
    {
    public:
        virtual void push(Task task) = 0;
        virtual void pushStop() = 0;
        virtual Task pop() = 0;
        virtual Task pop(bool isRunning) = 0;
    };

} // namespace TelemetrySchedulerImpl
