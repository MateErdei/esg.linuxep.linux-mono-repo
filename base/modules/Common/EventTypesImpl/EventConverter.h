/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#pragma once

#include "Common/EventTypes/CredentialEvent.h"
#include "Common/EventTypes/PortScanningEvent.h"

#include <Common/EventTypes/IEventType.h>
#include <Common/EventTypes/IEventConverter.h>
#include <Common/EventTypes/IEventException.h>

#include <memory>
#include <sstream>


namespace Common
{
    namespace EventTypes
    {
        class EventConverter : public Common::EventTypes::IEventConverter
        {
        public:
            const std::pair<std::string, std::string> eventToString(const Common::EventTypes::IEventType* eventType) override;

            template <class EventT>
            static std::unique_ptr<EventT> createEventFromString(const std::string& eventObjectAsString)
            {
                std::unique_ptr<EventT> event{new EventT()};
                event->fromString(eventObjectAsString);
                return event;
            }
        };
    }
}


