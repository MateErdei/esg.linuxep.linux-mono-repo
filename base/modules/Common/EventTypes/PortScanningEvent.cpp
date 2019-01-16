/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PortScanningEvent.h"
#include "EventStrings.h"

#include <Common/EventTypes/IEventException.h>
#include <Common/EventTypes/CommonEventData.h>

#include <PortScanning.capnp.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <sstream>

namespace
{
    Sophos::Journal::PortEvent::EventType convertToCapnEventType(Common::EventTypes::PortScanningEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Common::EventTypes::PortScanningEvent::EventType::closed:
                return Sophos::Journal::PortEvent::EventType::CLOSED;
            case Common::EventTypes::PortScanningEvent::EventType::connected:
                return Sophos::Journal::PortEvent::EventType::CONNECTED;
            case Common::EventTypes::PortScanningEvent::EventType::opened:
                return Sophos::Journal::PortEvent::EventType::OPENED;
            case Common::EventTypes::PortScanningEvent::EventType::scanned:
                return Sophos::Journal::PortEvent::EventType::SCANNED;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::PortScanningEvent::EventType, contained unknown type");
        }
    }

    Common::EventTypes::PortScanningEvent::EventType convertFromCapnEventType(Sophos::Journal::PortEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Sophos::Journal::PortEvent::EventType::CLOSED:
                return Common::EventTypes::PortScanningEvent::EventType::closed;
            case Sophos::Journal::PortEvent::EventType::CONNECTED:
                return Common::EventTypes::PortScanningEvent::EventType::connected;
            case Sophos::Journal::PortEvent::EventType::OPENED:
                return Common::EventTypes::PortScanningEvent::EventType::opened;
            case Sophos::Journal::PortEvent::EventType::SCANNED:
                return Common::EventTypes::PortScanningEvent::EventType::scanned;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::PortScanningEvent::EventType, contained unknown type");
        }
    }

}

Common::EventTypes::PortScanningEvent Common::EventTypes::createPortScanningEvent(Common::EventTypes::IpFlow& ipFlow,Common::EventTypes::PortScanningEvent::EventType eventType)
{
    Common::EventTypes::PortScanningEvent event = PortScanningEvent();
    event.setConnection(ipFlow);
    event.setEventType(eventType);

    return event;
}

namespace Common
{
    namespace EventTypes
    {


        const std::string PortScanningEvent::getEventTypeId() const
        {
            return Common::EventTypes::PortScanningEventName;
        }

        EventTypes::PortScanningEvent::EventType PortScanningEvent::getEventType() const
        {
            return m_eventType;
        }

        const EventTypes::IpFlow& PortScanningEvent::getConnection() const
        {
            return m_connection;
        }

        void PortScanningEvent::setEventType(EventTypes::PortScanningEvent::EventType m_eventType)
        {
            PortScanningEvent::m_eventType = m_eventType;
        }

        void PortScanningEvent::setConnection(const EventTypes::IpFlow& m_connection)
        {
            PortScanningEvent::m_connection = m_connection;
        }

        std::string PortScanningEvent::toString() const
        {
            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::PortEvent::Builder portEvent = message.initRoot<Sophos::Journal::PortEvent>();

            portEvent.setEventType(convertToCapnEventType(m_eventType));

            ::capnp::Data::Reader sourceAddress = ::capnp::Data::Reader(
                    reinterpret_cast<const ::capnp::byte*>(
                            m_connection.sourceAddress.address.data()),
                            m_connection.sourceAddress.address.size());

            portEvent.getConnection().getSourceAddress().setAddress(sourceAddress);
            portEvent.getConnection().getSourceAddress().setPort(m_connection.sourceAddress.port);

            ::capnp::Data::Reader destinationAddress = ::capnp::Data::Reader(
                    reinterpret_cast<const ::capnp::byte*>(
                            m_connection.destinationAddress.address.data()),
                            m_connection.destinationAddress.address.size());

            portEvent.getConnection().getDestinationAddress().setAddress(destinationAddress);
            portEvent.getConnection().getDestinationAddress().setPort(m_connection.destinationAddress.port);

            portEvent.getConnection().setProtocol(m_connection.protocol);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        void PortScanningEvent::fromString(const std::string& objectAsString)
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


                setEventType(convertFromCapnEventType(portEvent.getEventType()));

                auto connection = getConnection();

                ::capnp::Data::Reader sourceAddressData = portEvent.getConnection().getSourceAddress().getAddress();
                std::string sourceAddress(sourceAddressData.begin(), sourceAddressData.end());
                connection.sourceAddress.address = sourceAddress;

                connection.sourceAddress.port = portEvent.getConnection().getSourceAddress().getPort();

                ::capnp::Data::Reader destinationAddressData = portEvent.getConnection().getDestinationAddress().getAddress();
                std::string destinationAddress(destinationAddressData.begin(), destinationAddressData.end());
                connection.destinationAddress.address = destinationAddress;

                connection.destinationAddress.port = portEvent.getConnection().getDestinationAddress().getPort();

                connection.protocol = portEvent.getConnection().getProtocol();
                setConnection(connection);

            }
            catch(std::exception& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Error: failed to process capn PortScanningEvent string, " << ex.what();
                throw Common::EventTypes::IEventException(errorMessage.str());
            }
        }
    }
}

