/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#pragma once

#include <Common/EventTypes/CredentialEvent.h>
#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/EventTypes/ProcessEvent.h>
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
            EventTypes::CredentialEvent stringToCredentialEvent(const std::string& event) override;
            EventTypes::PortScanningEvent stringToPortScanningEvent(const std::string& event) override;
            EventTypes::ProcessEvent stringToProcessEvent(const std::string& event) override;

        private:

            template <class EventT>
            static EventT createEventFromString(const std::string& eventObjectAsString)
            {
                EventT event;
                event.fromString(eventObjectAsString);
                return std::move(event);
            }
        };
    }
}


