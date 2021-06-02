/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITaskQueue.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

namespace RemoteDiagnoseImpl
{

    class TaskQueue : public ITaskQueue
    {
    public:
        void push(Task task) override;
        Task pop() override;
        void pushStop() override;

    private:
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<Task> m_list;
    };

} // namespace RemoteDiagnoseImpl