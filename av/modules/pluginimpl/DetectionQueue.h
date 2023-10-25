// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ThreatDetected.h"

#include "Common/Threads/LockableBool.h"

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
        uint m_maxSize = 250;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::queue<scan_messages::ThreatDetected> m_list;
        Common::Threads::LockableBool m_stopRequested{false};
    private:
        bool isFullLocked(std::unique_lock<std::mutex>&);
        bool isEmptyLocked(std::unique_lock<std::mutex>&);

    public:
        void setMaxSize(uint);

        /** Attempts to push the detection to queue - fails if queue is full.
         *
         * @return true: successfully push, ThreatDetected moved
         * @return false: failed to push, ThreatDetected still valid
         */
        bool push(scan_messages::ThreatDetected&);

        std::optional<scan_messages::ThreatDetected> pop();
        bool isEmpty();
        bool isFull();
        void requestStop();
    };

} // namespace Plugin
