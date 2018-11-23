/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PortScanningEvent.h"

#include <Common/EventTypes/IEventException.h>
#include <Common/EventTypes/CommonEventData.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

namespace Common
{
    namespace EventTypesImpl
    {

        const std::string PortScanningEvent::getEventTypeId() const
        {
            return "Port Scanning";
        }

        EventTypes::IPortScanningEvent::EventType PortScanningEvent::getEventType() const
        {
            return m_eventType;
        }

        const EventTypes::IpFlow& PortScanningEvent::getConnection() const
        {
            return m_connection;
        }

        void PortScanningEvent::setEventType(EventTypes::IPortScanningEvent::EventType m_eventType)
        {
            PortScanningEvent::m_eventType = m_eventType;
        }

        void PortScanningEvent::setConnection(const EventTypes::IpFlow& m_connection)
        {
            PortScanningEvent::m_connection = m_connection;
        }

        Sophos::Journal::PortEvent::EventType PortScanningEvent::convertToCapnEventType(Common::EventTypes::IPortScanningEvent::EventType eventType)
        {
            switch(eventType)
            {
                case Common::EventTypes::IPortScanningEvent::EventType::closed:
                    return Sophos::Journal::PortEvent::EventType::CLOSED;
                case Common::EventTypes::IPortScanningEvent::EventType::connected:
                    return Sophos::Journal::PortEvent::EventType::CONNECTED;
                case Common::EventTypes::IPortScanningEvent::EventType::opened:
                    return Sophos::Journal::PortEvent::EventType::OPENED;
                case Common::EventTypes::IPortScanningEvent::EventType::scanned:
                    return Sophos::Journal::PortEvent::EventType::SCANNED;
                default:
                    throw Common::EventTypes::IEventException("Common::EventTypes::IPortScanningEvent::EventType, contained unknown type");
            }
        }

        Common::EventTypes::IPortScanningEvent::EventType PortScanningEvent::convertFromCapnEventType(Sophos::Journal::PortEvent::EventType eventType)
        {
            switch(eventType)
            {
                case Sophos::Journal::PortEvent::EventType::CLOSED:
                    return Common::EventTypes::IPortScanningEvent::EventType::closed;
                case Sophos::Journal::PortEvent::EventType::CONNECTED:
                    return Common::EventTypes::IPortScanningEvent::EventType::connected;
                case Sophos::Journal::PortEvent::EventType::OPENED:
                    return Common::EventTypes::IPortScanningEvent::EventType::opened;
                case Sophos::Journal::PortEvent::EventType::SCANNED:
                    return Common::EventTypes::IPortScanningEvent::EventType::scanned;
                default:
                    throw Common::EventTypes::IEventException("Common::EventTypes::IPortScanningEvent::EventType, contained unknown type");
            }
        }

        std::string PortScanningEvent::toString()
        {
            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::PortEvent::Builder portEvent = message.initRoot<Sophos::Journal::PortEvent>();

            portEvent.setEventType(convertToCapnEventType(m_eventType));
            portEvent.getConnection().getSourceAddress().setAddress(m_connection.sourceAddress.address);
            portEvent.getConnection().getSourceAddress().setPort(m_connection.sourceAddress.port);

            portEvent.getConnection().getDestinationAddress()Address().setAddress(m_connection.destinationAddress.address);
            portEvent.getConnection().getDestinationAddress().setPort(m_connection.destinationAddress.port);

            portEvent.getConnection().setProtocol(m_connection.protocol);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        PortScanningEvent PortScanningEvent::fromString(const std::string& objectAsString)
        {
            if(objectAsString.empty())
            {
                throw Common::EventTypes::IEventException("Invalid capn byte string, string is empty");
            }

            try
            {
                const kj::ArrayPtr<const capnp::word> view(
                        reinterpret_cast<const capnp::word*>(&(*std::begin(objectAsString))),
                        reinterpret_cast<const capnp::word*>(&(*std::end(objectAsString))));


                capnp::FlatArrayMessageReader message(view);
                Sophos::Journal::PortEvent::Reader portEvent = message.getRoot<Sophos::Journal::PortEvent>();

                Common::EventTypesImpl::PortScanningEvent event;

                event.setEventType(convertFromCapnEventType(portEvent.getEventType()));

                event.getConnection().sourceAddress.address = portEvent.getConnection().getSourceAddress().getAddress();
                event.getConnection().sourceAddress.port = portEvent.getConnection().getSourceAddress().getPort();

                event.getConnection().destinationAddress.address = portEvent.getConnection().getDestinationAddress().getAddress();
                event.getConnection().destinationAddress.port = portEvent.getConnection().getDestinationAddress().getPort();

                event.getConnection().protocol = portEvent.getConnection().getProtocol();
                return event;
            }
            catch(...)
            {
                throw Common::EventTypes::IEventException("Unknown exception: failed to process capn CredentialEvent string");
            }

        }

    }

}

