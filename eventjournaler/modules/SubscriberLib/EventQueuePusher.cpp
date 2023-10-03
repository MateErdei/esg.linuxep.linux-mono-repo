// Copyright 2021 Sophos Limited. All rights reserved.

#include "EventQueuePusher.h"

#include "JournalerCommon/TelemetryConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

namespace SubscriberLib
{

    SubscriberLib::EventQueuePusher::EventQueuePusher(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue) :
        m_eventQueue(eventQueue)
    {
    }

    void SubscriberLib::EventQueuePusher::handleEvent(JournalerCommon::Event event)
    {
        auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
        if (!m_eventQueue->push(event))
        {
            telemetryHelper.increment(JournalerCommon::Telemetry::telemetryDroppedAvEvents, 1L);
        }
    }
}