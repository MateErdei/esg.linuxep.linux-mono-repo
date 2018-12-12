/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CommonEventData.h"
#include "IEventType.h"


namespace Common
{
    namespace EventTypes
    {

    class PortScanningEvent : public Common::EventTypes::IEventType
        {
        public:

        enum EventType
        {
            opened = 0,
            closed = 1,
            connected = 2,
            scanned = 3
        };
            PortScanningEvent() = default;
            ~PortScanningEvent() = default;


            const std::string getEventTypeId() const override;
            EventType getEventType() const;
            const EventTypes::IpFlow& getConnection() const;

            void setEventType(EventType m_eventType);
            void setConnection(const EventTypes::IpFlow& m_connection);

            std::string toString() const override;
            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns PortScanningEvent object created from the capn byte string..
            */
            void fromString(const std::string& objectAsString);

        private:
            EventType m_eventType;
            Common::EventTypes::IpFlow m_connection;
        };

        Common::EventTypes::PortScanningEvent createPortScanningEvent(EventTypes::IpFlow& ipFlow, Common::EventTypes::PortScanningEvent::EventType eventType);
    }

}