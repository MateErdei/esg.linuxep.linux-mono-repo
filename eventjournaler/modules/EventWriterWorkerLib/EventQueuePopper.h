// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "modules/EventQueueLib/IEventQueuePopper.h"

#include <memory>

namespace EventWriterLib
{
    class EventQueuePopper : public EventQueueLib::IEventQueuePopper
    {
    public:
        explicit EventQueuePopper(const std::shared_ptr<EventQueueLib::IEventQueuePopper>& eventQueue);
        std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) override;

    private:
        std::shared_ptr<EventQueueLib::IEventQueuePopper> m_eventQueue;
    };
}
