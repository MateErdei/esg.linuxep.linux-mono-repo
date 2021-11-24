/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <set>
#include <mutex>

namespace Heartbeat
{
    class HeartbeatPinger
    {
    public:
        explicit HeartbeatPinger();
        virtual void ping();
        bool isAlive();
        virtual void pushDroppedEvent();
        virtual void setDroppedEventsMax(uint newMax);
        uint getNumDroppedEventsInLast24h();

    private:
        void trimDroppedEventsByTime(time_t now);
        void trimDroppedEventsBySize();
        time_t m_lastPinged = 0;
        uint m_droppedEventsMax = 10;
        std::multiset<time_t> m_droppedEvents{};
        std::mutex m_droppedEventsMutex;
    };

} // namespace Heartbeat