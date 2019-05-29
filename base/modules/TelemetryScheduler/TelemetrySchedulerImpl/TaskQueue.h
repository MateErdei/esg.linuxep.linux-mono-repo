/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SchedulerTask.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

namespace TelemetrySchedulerImpl
{
    class TaskQueue
    {
    public:
        void push(Task);
        Task pop();

    private:
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;
    };

} // namespace TelemetrySchedulerImpl
