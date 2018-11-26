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
            std::pair<std::string, std::string> eventToString(Common::EventTypes::IEventType* eventType) override;

            template <class EventT>
            EventT createEventFromString(const std::string& eventTypeAsString, const std::string& eventObjectAsString)
            {
                // ensure that only expected event types are processed.
                if (eventTypeAsString == "Detector.Credentials"
                || eventTypeAsString == "Detector.PortScanning")
                {
                    EventT event;
                    return event.fromString(eventObjectAsString);
                }

                std::stringstream errorMessage;
                errorMessage << "The Event Type passed in had an unknown eventTypeID: '" <<  eventTypeAsString << " '";
                throw Common::EventTypes::IEventException(errorMessage.str());
            }
        };
    }
}


