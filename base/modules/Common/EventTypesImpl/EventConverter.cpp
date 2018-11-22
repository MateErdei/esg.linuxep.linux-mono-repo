/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "EventConverter.h"
#include <Common/EventTypesImpl/CredentialEvent.h>

namespace Common
{
    namespace EventTypesImpl
    {
        std::pair<std::string, std::string> EventConverter::eventToString(Common::EventTypes::IEventType* eventType)
        {
            std::pair<std::string, std::string> eventData;
            eventData.first = eventType->getEventTypeId();
            eventData.second = eventType->toString();
            return eventData;
        }

    }
}