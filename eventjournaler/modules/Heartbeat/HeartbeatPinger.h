/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <set>

namespace Heartbeat
{
    class HeartbeatPinger
    {
    public:
        explicit HeartbeatPinger();
        void ping();
        bool isAlive();
        void pushDroppedEvent();
        uint getNumDroppedEventsInLast24h();

    private:
        void trimDroppedEvents();
        time_t m_lastPinged = 0;
        std::multiset<time_t> m_droppedEvents{};
    };

} // namespace Heartbeat