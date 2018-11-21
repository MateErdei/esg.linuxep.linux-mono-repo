/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************///

#include <Common/EventTypes/IEventType.h>
#include <Common/EventTypes/IEventConverter.h>
#include <memory>

#pragma once

namespace Common
{
    namespace EventTypesImpl
    {
        class EventConverter : public Common::EventTypes::IEventConverter
        {
        public:
            std::pair<std::string, std::string> eventToString(Common::EventTypes::IEventType* eventType) override;

            template <class EventT>
            EventT createEventFromString(std::string& eventTypeAsString, std::string& eventObjectAsString);


        };
    }
}


