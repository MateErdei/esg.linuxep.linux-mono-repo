// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "HeartbeatPinger.h"

#include "EventWriterWorkerLib/EventWriterWorker.h"
#include "SubscriberLib/Subscriber.h"

#include <Common/UtilityImpl/TimeUtils.h>

#include <set>

namespace Heartbeat
{
    HeartbeatPinger::HeartbeatPinger() = default;

    void HeartbeatPinger::ping()
    {
        m_lastPinged = Common::UtilityImpl::TimeUtils::getCurrTime();
    }

    bool HeartbeatPinger::isAlive()
    {
        // Threads need to ping at least once per X seconds, which we'll set to the max time a thread is asleep + a bit.
        long maxPingPeriod = std::max(EventWriterLib::EventWriterWorker::DEFAULT_QUEUE_SLEEP_INTERVAL_MS, SubscriberLib::Subscriber::DEFAULT_READ_LOOP_TIMEOUT_MS) + 3;
        return Common::UtilityImpl::TimeUtils::getCurrTime() - maxPingPeriod <= m_lastPinged;
    }

    void HeartbeatPinger::pushDroppedEvent()
    {
        std::lock_guard<std::mutex> lock{ m_droppedEventsMutex };
        time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
        m_droppedEvents.insert(now);
        trimDroppedEventsByTime(now);
        trimDroppedEventsBySize();
    }

    void HeartbeatPinger::trimDroppedEventsByTime(time_t now)
    {
        time_t oneDay = 60*60*24;
        time_t lowerBound = now - oneDay;
        time_t upperBound = now + 1;
        m_droppedEvents.erase(m_droppedEvents.begin(), m_droppedEvents.lower_bound(lowerBound));
        m_droppedEvents.erase(m_droppedEvents.upper_bound(upperBound), m_droppedEvents.end());
    }

    uint HeartbeatPinger::getNumDroppedEventsInLast24h()
    {
        std::lock_guard<std::mutex> lock{ m_droppedEventsMutex };
        trimDroppedEventsByTime(Common::UtilityImpl::TimeUtils::getCurrTime());
        return m_droppedEvents.size();
    }

    void HeartbeatPinger::setDroppedEventsMax(uint newMax)
    {
        m_droppedEventsMax = newMax;
    }

    void HeartbeatPinger::trimDroppedEventsBySize()
    {
        if (m_droppedEvents.size() > m_droppedEventsMax)
        {
            uint numToDelete = m_droppedEvents.size() - m_droppedEventsMax;
            auto it1 = m_droppedEvents.begin();
            auto it2 = m_droppedEvents.begin();
            std::advance(it2, numToDelete);
            m_droppedEvents.erase(it1,it2);
        }
    }

} // namespace Heartbeat