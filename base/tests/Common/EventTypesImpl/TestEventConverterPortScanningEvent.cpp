/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>

#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include "TestEventTypeHelper.h"

using namespace Common::EventTypes;

class TestEventConverterPortScanningEvent : public Tests::TestEventTypeHelper
{

public:

    TestEventConverterPortScanningEvent() = default;

};

TEST_F(TestEventConverterPortScanningEvent, testConstructorDoesNotThrow) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> p;
    EXPECT_NO_THROW(p = Common::EventTypes::constructEventConverter();); //NOLINT
}

TEST_F(TestEventConverterPortScanningEvent, testcreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedValues) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToPortScanningEvent(data.second);

    EXPECT_PRED_FORMAT2( portScanningEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverterPortScanningEvent, testcreatePortScanningEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter->stringToPortScanningEvent("Not Valid Capn String"), Common::EventTypes::IEventException); //NOLINT
}

TEST_F(TestEventConverterPortScanningEvent, testcreatePortScanningFromStringThrowsIfDataTypeStringIsEmpty) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter->stringToPortScanningEvent(""), Common::EventTypes::IEventException); //NOLINT
}