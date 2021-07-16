/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventQueuePopper.h"

namespace WriterLib
{

    WriterLib::EventQueuePopper::EventQueuePopper(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue) :
            m_eventQueue(eventQueue)
    {
    }

    std::optional<Common::ZeroMQWrapper::data_t> WriterLib::EventQueuePopper::getEvent(int timeoutInMilliseconds)
    {
        return m_eventQueue->pop(timeoutInMilliseconds);
    }
}