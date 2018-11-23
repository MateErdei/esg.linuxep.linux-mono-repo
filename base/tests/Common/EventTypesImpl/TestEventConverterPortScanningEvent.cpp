/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <Common/EventTypesImpl/PortScanningEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>


using namespace Common::EventTypesImpl;

class TestEventConverterPortScanningEvent : public ::testing::Test
{

public:

    TestEventConverterPortScanningEvent() = default;

    PortScanningEvent createDefaultPortScanningEvent()
    {
        PortScanningEvent event;

        auto connection = event.getConnection();
        connection.sourceAddress.address = "192.168.0.1";
        connection.sourceAddress.port = 80;
        connection.destinationAddress.address = "192.168.0.2";
        connection.destinationAddress.port = 80;
        connection.protocol = 25;
        event.setConnection(connection);
        event.setEventType(Common::EventTypesImpl::PortScanningEvent::opened);
        return event;
    }


    ::testing::AssertionResult portScanningEventIsEquivalent( const char* m_expr,
                                                              const char* n_expr,
                                                              const Common::EventTypesImpl::PortScanningEvent& expected,
                                                              const Common::EventTypesImpl::PortScanningEvent& resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";


        if ( expected.getConnection().sourceAddress.address != resulted.getConnection().sourceAddress.address )
        {
            return ::testing::AssertionFailure() << s.str() << "Connection source address differs";
        }

        if ( expected.getConnection().sourceAddress.port != resulted.getConnection().sourceAddress.port )
        {
            return ::testing::AssertionFailure() << s.str() << "Connection source port differs";
        }

        if ( expected.getConnection().destinationAddress.address != resulted.getConnection().destinationAddress.address )
        {
            return ::testing::AssertionFailure() << s.str() << "Connection destination address differs";
        }

        if ( expected.getConnection().destinationAddress.port != resulted.getConnection().destinationAddress.port )
        {
            return ::testing::AssertionFailure() << s.str() << "Connection destination port differs";
        }

        if ( expected.getConnection().protocol != resulted.getConnection().protocol )
        {
            return ::testing::AssertionFailure() << s.str() << "Connection protocal differs";
        }

        if ( expected.getEventType() != resulted.getEventType() )
        {
            return ::testing::AssertionFailure() << s.str() << "Event type differs";
        }

        return ::testing::AssertionSuccess();
    }


};


TEST_F(TestEventConverterPortScanningEvent, testConstructorDoesNotThrow) //NOLINT
{
    EXPECT_NO_THROW(EventConverter eventConverter;);
}

TEST_F(TestEventConverterPortScanningEvent, testcreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedValues) //NOLINT
{
    EventConverter converter;
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    auto eventActual = converter.createEventFromString<PortScanningEvent>(data.first, data.second);

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, eventActual);
}


TEST_F(TestEventConverterPortScanningEvent, testcreateCredentialEventFromStringThrowsIfDataIsEmptyString) //NOLINT
{
    EventConverter converter;
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter.createEventFromString<PortScanningEvent>("Port Scanning", ""), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterPortScanningEvent, testcreateCredentialEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    EventConverter converter;
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter.createEventFromString<PortScanningEvent>("Port Scanning", "Not Valid Capn String"), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterPortScanningEvent, testcreateCredentialEventFromStringThrowsIfObjectTypeIsNotKnown) //NOLINT
{
    EventConverter converter;
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter.createEventFromString<PortScanningEvent>("Not a Known Type", ""), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterPortScanningEvent, testcreateCredentialEventFromStringThrowsIfObjectTypeStringIsEmpty) //NOLINT
{
    EventConverter converter;
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter.createEventFromString<PortScanningEvent>("", ""), Common::EventTypes::IEventException);
}