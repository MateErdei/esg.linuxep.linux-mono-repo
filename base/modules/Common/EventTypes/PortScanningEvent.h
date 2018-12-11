/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CommonEventData.h"
#include "IPortScanningEvent.h"


namespace Common
{
    namespace EventTypesImpl
    {
        class PortScanningEvent : public Common::EventTypes::IPortScanningEvent
        {
        public:
            PortScanningEvent() = default;
            ~PortScanningEvent() = default;


            const std::string getEventTypeId() const override;
            EventType getEventType() const override;
            const EventTypes::IpFlow& getConnection() const override;

            void setEventType(EventType m_eventType) override;
            void setConnection(const EventTypes::IpFlow& m_connection) override;

            const std::string toString() const override;
            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns PortScanningEvent object created from the capn byte string..
            */
            void fromString(const std::string& objectAsString);

        private:
            Common::EventTypes::IPortScanningEvent::EventType m_eventType;
            Common::EventTypes::IpFlow m_connection;
        };

    }

}