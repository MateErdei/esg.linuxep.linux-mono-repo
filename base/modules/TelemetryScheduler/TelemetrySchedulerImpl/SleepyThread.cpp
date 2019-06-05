/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SleepyThread.h"

namespace TelemetrySchedulerImpl
{
    SleepyThread::SleepyThread(size_t sleepUntil, Task task, std::shared_ptr<TaskQueue> queue) :
        m_sleepUntil(sleepUntil),
        m_task(task),
        m_queue(std::move(queue)),
        m_finished(false)
    {
    }

    void SleepyThread::run()
    {
        announceThreadStarted();

        while (!stopRequested())
        {
            auto now = std::chrono::system_clock::now();
            size_t nowInSecondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

            if (nowInSecondsSinceEpoch >= m_sleepUntil)
            {
                m_finished = true;
                m_queue->push(m_task);
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
} // namespace TelemetrySchedulerImpl
