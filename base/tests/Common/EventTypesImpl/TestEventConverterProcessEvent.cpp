/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>

#include <Common/EventTypes/ProcessEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include "TestEventTypeHelper.h"

using namespace Common::EventTypes;

class TestEventConverterProcessEvent : public Tests::TestEventTypeHelper
{

public:

    TestEventConverterProcessEvent() = default;

};

TEST_F(TestEventConverterProcessEvent, testcreateProcessEventFromStringCanCreateProcessEventObjectWithExpectedValues) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    ProcessEvent eventExpected = createDefaultProcessEvent();

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToProcessEvent(data.second);

    EXPECT_PRED_FORMAT2( processEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverterProcessEvent, testcreateProcessEventFromStringCanCreateProcessEventObjectWithExpectedNonLatinCharacterValues) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    ProcessEvent eventExpected = createDefaultProcessEvent();
    eventExpected.setCmdLine("いいい");

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToProcessEvent(data.second);

    EXPECT_PRED_FORMAT2( processEventIsEquivalent, eventExpected, eventActual);
}


TEST_F(TestEventConverterProcessEvent, testcreateProcessEventFromStringThrowsIfDataInvalidCapnString) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    EXPECT_THROW(converter->stringToProcessEvent("Not Valid Capn String"), Common::EventTypes::IEventException); //NOLINT
}

TEST_F(TestEventConverterProcessEvent, testcreateProcessEventFromStringThrowsIfDataTypeStringIsEmpty) //NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    EXPECT_THROW(converter->stringToProcessEvent(""), Common::EventTypes::IEventException); //NOLINT
}

TEST_F(TestEventConverterProcessEvent, testcreateProcessEventForStartProcess) //NOLINT
{
    // test to prove that incomplete data is still valid a event, i.e a start process event will not have an end time.

    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();

    ProcessEvent event;

    event.setEventType(Common::EventTypes::ProcessEvent::EventType::start);

    Common::EventTypes::SophosPid sophosPid;
    sophosPid.pid = 1084;
    sophosPid.timestamp = 123123123123;
    event.setSophosPid(sophosPid);

    Common::EventTypes::SophosPid parentSophosPid;
    parentSophosPid.pid = 10084;
    parentSophosPid.timestamp = 123123123123;
    event.setParentSophosPid(parentSophosPid);

    Common::EventTypes::SophosTid parentSophosTid;
    parentSophosTid.tid = 234234;
    parentSophosTid.timestamp = 123123123123;
    event.setParentSophosTid(parentSophosTid);


    Common::EventTypes::OptionalUInt64 fileSize;
    fileSize.value = 123;
    event.setFileSize(fileSize);

    event.setFlags(48);
    event.setSessionId(312);
    event.setSid("sid");

    Common::EventTypes::UserSid userSid;
    userSid.username = "testUser";
    userSid.domain = "testDomain";
    event.setProcessOwnerUserSid(userSid);

    Common::EventTypes::Pathname pathname;
    pathname.flags = 12;
    pathname.fileSystemType = 452;
    pathname.driveLetter = 6;
    pathname.pathname = "/name/of/path";

    Common::EventTypes::TextOffsetLength openName;
    openName.length = 23;
    openName.offset = 22;
    pathname.openName = openName;

    Common::EventTypes::TextOffsetLength volumeName;
    volumeName.length = 21;
    volumeName.offset = 20;
    pathname.volumeName = volumeName;

    Common::EventTypes::TextOffsetLength shareName;
    shareName.length = 19;
    shareName.offset = 18;
    pathname.shareName = shareName;

    Common::EventTypes::TextOffsetLength extensionName;
    extensionName.length = 17;
    extensionName.offset = 16;
    pathname.extensionName = extensionName;

    Common::EventTypes::TextOffsetLength streamName;
    streamName.length = 15;
    streamName.offset = 14;
    pathname.streamName = streamName;

    Common::EventTypes::TextOffsetLength finalComponentName;
    finalComponentName.length = 13;
    finalComponentName.offset = 12;
    pathname.finalComponentName = finalComponentName;

    Common::EventTypes::TextOffsetLength parentDirName;
    parentDirName.length = 11;
    parentDirName.offset = 10;
    pathname.parentDirName = parentDirName;

    event.setPathname(pathname);

    event.setCmdLine("example cmd line");

    event.setSha256("somesha256");
    event.setSha1("somesha1");

    std::pair<std::string, std::string> data = converter->eventToString(&event);

    auto eventActual = converter->stringToProcessEvent(data.second);

    EXPECT_PRED_FORMAT2( processEventIsEquivalent, event, eventActual);

}
