/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>

#include <Common/EventTypes/CredentialEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/TestHelpers/TestEventTypeHelper.h>

using namespace Common::EventTypes;

class TestEventConverterCredentialEvent : public Tests::TestEventTypeHelper
{

public:

    TestEventConverterCredentialEvent() = default;

};

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedValues) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToCredentialEvent(data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringCanCreateCredentialEventObjectWithExpectedNonLatinCharacterValues) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    CredentialEvent eventExpected = createDefaultCredentialEvent();
    eventExpected.setGroupName("いいい");

    Common::EventTypes::UserSid subjectUserId;

    subjectUserId.username = eventExpected.getSubjectUserSid().username;
    subjectUserId.domain = "いいい";
    eventExpected.setSubjectUserSid(subjectUserId);

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToCredentialEvent(data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    EXPECT_THROW(converter->stringToCredentialEvent("Not Valid Capn String"), Common::EventTypes::IEventException); //NOLINT
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventFromStringThrowsIfDataTypeStringIsEmpty) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    EXPECT_THROW(converter->stringToCredentialEvent(""), Common::EventTypes::IEventException); //NOLINT
}

TEST_F(TestEventConverterCredentialEvent, testcreateCredentialEventForAddUser) //NOLINT
{
    // test to prove that incomplete data is still valid a event, i.e addUser event will not generate all
    // the data that could be stored.
    // network information will not be added, and should not be an issue.

    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    CredentialEvent eventExpected;
    eventExpected.setEventType(Common::EventTypes::CredentialEvent::EventType::created);
    eventExpected.setSessionType(Common::EventTypes::CredentialEvent::SessionType::interactive);
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

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToCredentialEvent(data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, eventActual);

}
