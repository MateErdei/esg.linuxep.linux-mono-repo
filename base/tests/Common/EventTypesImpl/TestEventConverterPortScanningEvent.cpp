/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>

#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/TestHelpers/TestEventTypeHelper.h>

using namespace Common::EventTypes;

class TestEventConverterPortScanningEvent : public Tests::TestEventTypeHelper
{

public:

    TestEventConverterPortScanningEvent() = default;

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

    auto eventActual = EventConverter::createEventFromString<PortScanningEvent>(data.second);

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, *eventActual);
}

TEST_F(TestEventConverterPortScanningEvent, testcreatePortScanningEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(EventConverter::createEventFromString<PortScanningEvent>("Not Valid Capn String"), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterPortScanningEvent, testcreatePortScanningFromStringThrowsIfDataTypeStringIsEmpty) //NOLINT
{
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(EventConverter::createEventFromString<PortScanningEvent>(""), Common::EventTypes::IEventException);
}