/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#pragma once

#include "CredentialEvent.h"
#include "PortScanningEvent.h"

#include <Common/EventTypes/IEventType.h>
#include <Common/EventTypes/IEventConverter.h>
#include <Common/EventTypes/IEventException.h>

#include <memory>
#include <sstream>


namespace Common
{
    namespace EventTypesImpl
    {
        class EventConverter : public Common::EventTypes::IEventConverter
        {
        public:
            const std::pair<std::string, std::string> eventToString(const Common::EventTypes::IEventType* eventType) override;

            template <class EventT>
            static EventT createEventFromString(const std::string& eventObjectAsString)
            {
                EventT event;
                return event.fromString(eventObjectAsString);               
            }
        };
    }
}


