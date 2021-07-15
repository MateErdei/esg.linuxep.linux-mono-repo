/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventQueuePusher.h"
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/TelemetryConsts.h>

namespace SubscriberLib
{

    SubscriberLib::EventQueuePusher::EventQueuePusher(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue) :
            m_eventQueue(eventQueue)
    {
    }

    void SubscriberLib::EventQueuePusher::push(Common::ZeroMQWrapper::data_t event)
    {
        auto& telemetryHelper = Common::Telemetry::TelemetryHelper::getInstance();
        if (!m_eventQueue->push(event))
        {
            telemetryHelper.increment(Plugin::Telemetry::telemetryDroppedAvEvents, ulong(1));
        }
    }
}