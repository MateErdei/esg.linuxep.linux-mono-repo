// Copyright 2021 Sophos Limited. All rights reserved.

#include "IEventHandler.h"

#include "EventQueueLib/EventQueue.h"
#include "Heartbeat/HeartbeatPinger.h"

#include "Common/ZeroMQWrapper/IReadable.h"

#include <optional>

namespace SubscriberLib
{
    class EventQueuePusher : public IEventHandler
    {
    public:
        EventQueuePusher(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue);
        void handleEvent(JournalerCommon::Event event) override;

    private:
        std::shared_ptr<EventQueueLib::IEventQueue> m_eventQueue;
    };
}