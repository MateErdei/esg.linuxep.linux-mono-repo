// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ThreatDetected.h"

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <optional>

namespace Plugin
{
    class DetectionQueue
    {
        uint m_maxSize = 1024;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::queue<scan_messages::ThreatDetected> m_list;
        std::atomic_bool m_stopRequested = false;

    public:
        void setMaxSize(uint);
        void push(scan_messages::ThreatDetected);
        std::optional<scan_messages::ThreatDetected> pop();
        bool isEmpty();
        bool isFull();
        void requestStop();
    };

} // namespace Plugin
