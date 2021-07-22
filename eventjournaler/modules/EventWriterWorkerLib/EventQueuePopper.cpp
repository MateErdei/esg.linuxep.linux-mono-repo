/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventQueuePopper.h"

namespace EventWriterLib
{
    EventWriterLib::EventQueuePopper::EventQueuePopper(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue) :
        m_eventQueue(eventQueue)
    {
    }

    std::optional<JournalerCommon::Event> EventWriterLib::EventQueuePopper::getEvent(int timeoutInMilliseconds)
    {
        return m_eventQueue->pop(timeoutInMilliseconds);
    }
}