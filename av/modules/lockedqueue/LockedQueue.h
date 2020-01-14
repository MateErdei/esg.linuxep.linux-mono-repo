/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Threads/NotifyPipe.h"

#include <deque>
#include <mutex>
#include <optional>

namespace lockedqueue
{
    template<class T>
    class LockedQueue
    {
    public:
        void add(T item);
        std::optional<T> get();
    private:
        std::mutex m_lock;
        std::deque<T> m_queue;
        Common::Threads::NotifyPipe m_notifyPipe;

    };
}
