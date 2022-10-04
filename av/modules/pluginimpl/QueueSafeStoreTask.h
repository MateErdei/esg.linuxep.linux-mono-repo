// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ServerThreatDetected.h"

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <optional>

namespace Plugin
{
    class QueueSafeStoreTask
    {
        uint m_maxSize = 1024;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::queue<scan_messages::ServerThreatDetected> m_list;
        std::atomic_bool m_stopRequested = false;

    public:
        void setMaxSize(uint);
        void push(scan_messages::ServerThreatDetected);
        std::optional<scan_messages::ServerThreatDetected> pop();
        bool isEmpty();
        bool isFull();
        void requestStop();
    };

} // namespace Plugin
