/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#pragma once

#include "CredentialEvent.h"

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
            std::pair<std::string, std::string> eventToString(Common::EventTypes::IEventType* eventType) override;

            template <class EventT>
            EventT createEventFromString(const std::string& eventTypeAsString, const std::string& eventObjectAsString)
            {
                if (eventTypeAsString == "Credentials")
                {
                    EventT event;
                    event = Common::EventTypesImpl::CredentialEvent();
                    Common::EventTypesImpl::CredentialEvent eventCreated = event.fromString(eventObjectAsString);
                    return eventCreated;
                }

                std::stringstream errorMessage;
                errorMessage << "The Event Type passed in had an unknown eventTypeID: '" <<  eventTypeAsString << " '";
                throw Common::EventTypes::IEventException(errorMessage.str());
            }
        };
    }
}


