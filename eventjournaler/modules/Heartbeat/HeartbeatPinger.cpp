/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "HeartbeatPinger.h"
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
        return Common::UtilityImpl::TimeUtils::getCurrTime() - 15 <= m_lastPinged;
    }

    void HeartbeatPinger::pushDroppedEvent()
    {
        m_droppedEvents.insert(Common::UtilityImpl::TimeUtils::getCurrTime());
    }

    void HeartbeatPinger::trimDroppedEvents()
    {
        time_t oneDay = 60*60*24;
        time_t lowerBound = Common::UtilityImpl::TimeUtils::getCurrTime() - oneDay;
        std::multiset<time_t> newDroppedEvents;
        newDroppedEvents.insert(m_droppedEvents.lower_bound(lowerBound), m_droppedEvents.end());
        m_droppedEvents = newDroppedEvents;
    }

    uint HeartbeatPinger::getNumDroppedEventsInLast24h()
    {
        trimDroppedEvents();
        return m_droppedEvents.size();
    }



} // namespace Heartbeat