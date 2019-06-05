/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITaskQueue.h"

#include <Common/Threads/AbstractThread.h>

#include <atomic>
#include <string>

namespace TelemetrySchedulerImpl
{
    class SleepyThread : public Common::Threads::AbstractThread
    {
    public:
         /**
          * Construct thread for delaying queueing of a task.
          * @param sleepUntil time to queue task
          * @param task task to queue after delay
          * @param queue queue to add task to
          */
        SleepyThread(std::chrono::system_clock::time_point sleepUntil, Task task, std::shared_ptr<ITaskQueue> queue);

        /**
         * Run thread so that it sleeps until it's time to queue the task.
         * If time is in the past, then the thread queues the task immediately.
         */
        void run() override;

        bool finished() { return m_finished; }

    private:
        std::chrono::system_clock::time_point m_sleepUntil;
        Task m_task;
        std::shared_ptr<ITaskQueue> m_queue;
        std::atomic<bool> m_finished;
    };
} // namespace TelemetrySchedulerImpl
