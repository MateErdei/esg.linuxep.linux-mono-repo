/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <optional>
#include "modules/EventQueueLib/EventQueue.h"
#include "IEventQueuePusher.h"
#include "Common/ZeroMQWrapper/IReadable.h"

namespace SubscriberLib
{
    class EventQueuePusher : public IEventQueuePusher
    {
    public:
        EventQueuePusher(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue);
        void push(Common::ZeroMQWrapper::data_t event) override;

    private:
        std::shared_ptr<EventQueueLib::IEventQueue> m_eventQueue;
    };
}