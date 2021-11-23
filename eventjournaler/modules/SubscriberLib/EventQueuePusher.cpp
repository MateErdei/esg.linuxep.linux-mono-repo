/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventQueuePusher.h"
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/TelemetryConsts.h>

namespace SubscriberLib
{

    SubscriberLib::EventQueuePusher::EventQueuePusher(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue,
                                                      std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger) :
        m_eventQueue(eventQueue),
        m_heartbeatPinger(heartbeatPinger)
    {
    }

    void SubscriberLib::EventQueuePusher::handleEvent(JournalerCommon::Event event)
    {
        auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
        if (!m_eventQueue->push(event))
        {
            telemetryHelper.increment(Plugin::Telemetry::telemetryDroppedAvEvents, 1L);
            m_heartbeatPinger->pushDroppedEvent();
        }
    }
}