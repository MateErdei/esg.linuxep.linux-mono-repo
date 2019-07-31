/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SleepyThread.h"

namespace TelemetrySchedulerImpl
{
    SleepyThread::SleepyThread(
        std::chrono::system_clock::time_point sleepUntil,
        SchedulerTask task,
        std::shared_ptr<ITaskQueue> queue) :
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
            if (std::chrono::system_clock::now() >= m_sleepUntil)
            {
                m_finished = true;
                m_queue->push(m_task);
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
} // namespace TelemetrySchedulerImpl
