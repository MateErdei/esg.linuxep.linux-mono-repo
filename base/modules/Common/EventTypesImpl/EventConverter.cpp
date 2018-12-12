/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventConverter.h"

namespace Common
{
    namespace EventTypesImpl
    {
        const std::pair<std::string, std::string> EventConverter::eventToString(const Common::EventTypes::IEventType* eventType)
        {
            std::pair<std::string, std::string> eventData;
            eventData.first = eventType->getEventTypeId();
            eventData.second = eventType->toString();
            return eventData;
        }

        EventTypes::CredentialEvent EventConverter::stringToCredentialEvent(const std::string& event)
        {
            return createEventFromString<EventTypes::CredentialEvent>(event);
        }

        EventTypes::PortScanningEvent EventConverter::stringToPortScanningEvent(const std::string& event)
        {
            return createEventFromString<EventTypes::PortScanningEvent>(event);
        }
    }

    namespace EventTypes
    {
        std::unique_ptr<IEventConverter> constructEventConverter()
        {
            return std::unique_ptr<IEventConverter>(new EventTypesImpl::EventConverter());
        }
    }
}