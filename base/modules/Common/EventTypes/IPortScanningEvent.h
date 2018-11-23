/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IEventType.h"
#include "Common/EventTypes/CommonEventData.h"

namespace Common
{
    namespace EventTypes
    {
        class IPortScanningEvent : public IEventType
        {
        public:
            enum EventType
            {
                opened = 0,
                closed = 1,
                connected = 2,
                scanned = 3
            };

            virtual EventType getEventType() const = 0;
            virtual const EventTypes::IpFlow& getConnection() const = 0;
            virtual void setEventType(EventType m_eventType) = 0;
            virtual void setConnection(const EventTypes::IpFlow& m_connection) = 0;

        };
    }
}