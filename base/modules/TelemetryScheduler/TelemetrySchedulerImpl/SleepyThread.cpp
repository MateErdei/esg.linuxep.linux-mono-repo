// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "SleepyThread.h"

#include <poll.h>

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
        struct pollfd fds[] {
            { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        };
        
        announceThreadStarted();

        while (!stopRequested())
        {
            auto now = std::chrono::system_clock::now();
            if (now >= m_sleepUntil)
            {
                m_finished = true;
                m_queue->push(m_task);
                break;
            }

            struct timespec timeout{};
            auto time_till_deadline = m_sleepUntil - now;
            auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(time_till_deadline).count();
            nanoseconds /= 2; // fudge factor so that we wake up before the deadline
            constexpr long nanoseconds_in_second = 1000000000;
            nanoseconds = std::max(nanoseconds, nanoseconds_in_second);
            timeout.tv_nsec = nanoseconds % 1000000000;
            timeout.tv_sec  = nanoseconds / 1000000000;

            auto ret = ::ppoll(fds, std::size(fds), &timeout, nullptr);
            std::ignore = ret; // check handled by while loop
        }
    }
} // namespace TelemetrySchedulerImpl
