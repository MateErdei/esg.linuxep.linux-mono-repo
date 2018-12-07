/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>

#include <Common/EventTypesImpl/CredentialEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/TestHelpers/TestEventTypeHelper.h>

using namespace Common::EventTypesImpl;

class TestEventConverterCredentialEvent : public Tests::TestEventTypeHelper
{

public:

    TestEventConverterCredentialEvent() = default;

};


TEST_F(TestEventConverterCredentialEvent, testConstructorDoesNotThrow) //NOLINT
{
    EXPECT_NO_THROW(EventConverter eventConverter;);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedValues) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    auto eventActual = EventConverter::createEventFromString<CredentialEvent>(data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, *eventActual);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedNonLatinCharacterValues) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();
    eventExpected.setGroupName("いいい");

    Common::EventTypes::UserSid subjectUserId;

    subjectUserId.username = eventExpected.getSubjectUserSid().username;
    subjectUserId.domain = "いいい";
    eventExpected.setSubjectUserSid(subjectUserId);

    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    auto eventActual = EventConverter::createEventFromString<CredentialEvent>(data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, *eventActual);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    EXPECT_THROW(EventConverter::createEventFromString<CredentialEvent>("Not Valid Capn String"), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringThrowsIfDataTypeStringIsEmpty) //NOLINT
{
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    EXPECT_THROW(EventConverter::createEventFromString<CredentialEvent>(""), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventForAddUser)
{
    // test to prove that incomplete data is still valid a event, i.e addUser event will not generate all
    // the data that could be stored.
    // network information will not be added, and should not be an issue.

    EventConverter converter;
    CredentialEvent eventExpected;
    eventExpected.setEventType(Common::EventTypes::EventType::created);
    eventExpected.setSessionType(Common::EventTypes::SessionType::interactive);
    auto subjectUserID = eventExpected.getSubjectUserSid();
    subjectUserID.username = "TestUser";
    eventExpected.setSubjectUserSid(subjectUserID);

    auto targetUserID = eventExpected.getTargetUserSid();
    targetUserID.username = "TestUser";
    eventExpected.setTargetUserSid(targetUserID);

    eventExpected.setGroupName("TestGroup");
    eventExpected.setGroupId(1002);
    eventExpected.setTimestamp(123123);
    eventExpected.setLogonId(1234);

    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    auto eventActual = EventConverter::createEventFromString<CredentialEvent>(data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, *eventActual);

}
