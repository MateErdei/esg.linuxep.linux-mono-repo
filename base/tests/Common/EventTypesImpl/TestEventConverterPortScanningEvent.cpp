/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestEventTypeHelper.h"

#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <gmock/gmock.h>

using namespace Common::EventTypes;

class TestEventConverterPortScanningEvent : public Tests::TestEventTypeHelper
{
public:
    TestEventConverterPortScanningEvent() = default;
};

TEST_F(TestEventConverterPortScanningEvent, testConstructorDoesNotThrow) // NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> p;
    EXPECT_NO_THROW(p = Common::EventTypes::constructEventConverter();); // NOLINT
}

TEST_F( // NOLINT
    TestEventConverterPortScanningEvent,
    testCreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedValues)
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToPortScanningEvent(data.second);

    EXPECT_PRED_FORMAT2(portScanningEventIsEquivalent, eventExpected, eventActual);
}

TEST_F( // NOLINT
    TestEventConverterPortScanningEvent,
    testCreatePortScanningEventFromStringThrowsIfDataInvalidCapnString)
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW( // NOLINT
        converter->stringToPortScanningEvent("Not Valid Capn String"),
        Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterPortScanningEvent, testCreatePortScanningFromStringThrowsIfDataTypeStringIsEmpty) // NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    PortScanningEvent eventExpected = createDefaultPortScanningEvent();

    EXPECT_THROW(converter->stringToPortScanningEvent(""), Common::EventTypes::IEventException); // NOLINT
}