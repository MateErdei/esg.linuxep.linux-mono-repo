/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <Common/EventTypesImpl/CredentialEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>


using namespace Common::EventTypesImpl;

class TestEventConverter : public ::testing::Test
{

public:

    TestEventConverter() = default;

    CredentialEvent createDefaultCredentialEvent()
    {
        CredentialEvent event;
        event.setSessionType(Common::EventTypes::SessionType::interactive);
        event.setLogonId(1000);
        event.setTimestamp(123123123);
        event.setGroupId(1001);
        event.setGroupName("TestGroup");

        Common::EventTypes::UserSid subjectUserId;
        subjectUserId.username = "TestSubUser";
        subjectUserId.domain = "sophos.com";
        event.setSubjectUserSid(subjectUserId);

        Common::EventTypes::UserSid targetUserId;
        targetUserId.username = "TestTarUser";
        targetUserId.domain = "sophosTarget.com";
        event.setTargetUserSid(targetUserId);

        Common::EventTypes::NetworkAddress network;
        network.address = "sophos.com:400";
        event.setRemoteNetworkAccess(network);

        return event;
    }

    ::testing::AssertionResult credentialEventIsEquivalent( const char* m_expr,
                                                              const char* n_expr,
                                                              const Common::EventTypesImpl::CredentialEvent& expected,
                                                              const Common::EventTypesImpl::CredentialEvent& resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if ( expected.getSessionType() != resulted.getSessionType())
        {
            return ::testing::AssertionFailure() << s.str() << "SessionType differs";
        }

        if ( expected.getEventType() != resulted.getEventType())
        {
            return ::testing::AssertionFailure() << s.str() << "EventType differs";
        }

        if (expected.getEventTypeId() != resulted.getEventTypeId())
        {
            return ::testing::AssertionFailure() << s.str() << "EventTypeId differs";
        }

        if (expected.getSubjectUserSid().domain != resulted.getSubjectUserSid().domain)
        {
            return ::testing::AssertionFailure() << s.str() << "SubjectUserSid domain differs";
        }

        if (expected.getSubjectUserSid().username != resulted.getSubjectUserSid().username)
        {
            return ::testing::AssertionFailure() << s.str() << "SubjectUserSid username differs";
        }

        if (expected.getTargetUserSid().domain != resulted.getTargetUserSid().domain)
        {
            return ::testing::AssertionFailure() << s.str() << "TargetUserSid domain differs";
        }

        if (expected.getTargetUserSid().username != resulted.getTargetUserSid().username)
        {
            return ::testing::AssertionFailure() << s.str() << "TargetUserSid username differs";
        }

        if (expected.getGroupId() != resulted.getGroupId())
        {
            return ::testing::AssertionFailure() << s.str() << "GroupId differs";
        }

        if (expected.getGroupName() != resulted.getGroupName())
        {
            return ::testing::AssertionFailure() << s.str() << "GroupName differs";
        }

        if (expected.getTimestamp() != resulted.getTimestamp())
        {
            return ::testing::AssertionFailure() << s.str() << "Timestamp differs";
        }

        if (expected.getLogonId() != resulted.getLogonId())
        {
            return ::testing::AssertionFailure() << s.str() << "logonId differs";
        }

        if (expected.getRemoteNetworkAccess().address != resulted.getRemoteNetworkAccess().address)
        {
            return ::testing::AssertionFailure() << s.str() << "RemoteNetworkAccess address differs";
        }

        return ::testing::AssertionSuccess();
    }

};


TEST_F(TestEventConverter, testConstructorDoesNotThrow) //NOLINT
{
    EXPECT_NO_THROW(EventConverter eventConverter;);
}

TEST_F(TestEventConverter, testcreateEventFromStringCanCreateCredentialEventObjectWithExpectedValues) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    auto eventActual = converter.createEventFromString<CredentialEvent>(data.first, data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverter, testcreateEventFromStringCanCreateCredentialEventObjectWithExpectedNonLatinCharacterValues) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();
    eventExpected.setGroupName("いいい");

    Common::EventTypes::UserSid subjectUserId;

    subjectUserId.username = eventExpected.getSubjectUserSid().username;
    subjectUserId.domain = "いいい";
    eventExpected.setSubjectUserSid(subjectUserId);

    std::pair<std::string, std::string> data = converter.eventToString(&eventExpected);

    auto eventActual = converter.createEventFromString<CredentialEvent>(data.first, data.second);

    EXPECT_PRED_FORMAT2( credentialEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverter, testcreateEventFromStringThrowsIfDataIsEmptyString) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    EXPECT_THROW(converter.createEventFromString<CredentialEvent>("Credentials", ""), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverter, testcreateEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    EXPECT_THROW(converter.createEventFromString<CredentialEvent>("Credentials", "Not Valid Capn String"), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverter, testcreateEventFromStringThrowsIfObjectTypeIsNotKnown) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    EXPECT_THROW(converter.createEventFromString<CredentialEvent>("Not a Known Type", ""), Common::EventTypes::IEventException);
}

TEST_F(TestEventConverter, testcreateEventFromStringThrowsIfObjectTypeStringIsEmpty) //NOLINT
{
    EventConverter converter;
    CredentialEvent eventExpected = createDefaultCredentialEvent();

    EXPECT_THROW(converter.createEventFromString<CredentialEvent>("", ""), Common::EventTypes::IEventException);
}