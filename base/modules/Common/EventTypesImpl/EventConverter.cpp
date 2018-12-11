/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "EventConverter.h"

namespace Common
{
    namespace EventTypes
    {
        const std::pair<std::string, std::string> EventConverter::eventToString(const Common::EventTypes::IEventType* eventType)
        {
            std::pair<std::string, std::string> eventData;
            eventData.first = eventType->getEventTypeId();
            eventData.second = eventType->toString();
            return eventData;
        }

    }
}