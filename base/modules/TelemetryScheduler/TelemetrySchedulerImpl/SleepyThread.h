/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SchedulerTask.h"
#include "TaskQueue.h"

#include <Common/Threads/AbstractThread.h>

#include <atomic>
#include <string>

namespace TelemetrySchedulerImpl
{
    class SleepyThread : public Common::Threads::AbstractThread
    {
    public:
         /**
          *
          * @param sleepUntil time to delay before queueing task
          * @param task task to queue after delay
          * @param queue queue to add task to
          */
        SleepyThread(size_t sleepUntil, Task task, std::shared_ptr<TaskQueue> queue);

        void run() override;

        bool finished() { return m_finished; }

    private:
        size_t m_sleepUntil;
        Task m_task;
        std::shared_ptr<TaskQueue> m_queue;
        std::atomic<bool> m_finished;
    };
} // namespace TelemetrySchedulerImpl
