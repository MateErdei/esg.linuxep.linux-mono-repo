/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITaskQueue.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

namespace TelemetrySchedulerImpl
{
    class TaskQueue : public ITaskQueue
    {
    public:
        void push(SchedulerTask) override;
        void pushPriority(SchedulerTask) override;
        SchedulerTask pop() override;

    private:
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<SchedulerTask> m_list;
    };

} // namespace TelemetrySchedulerImpl