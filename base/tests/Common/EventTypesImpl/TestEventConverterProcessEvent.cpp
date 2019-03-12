/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestEventTypeHelper.h"

#include <Common/EventTypes/ProcessEvent.h>
#include <Common/EventTypesImpl/EventConverter.h>
#include <gmock/gmock.h>

using namespace Common::EventTypes;

class TestEventConverterProcessEvent : public Tests::TestEventTypeHelper
{
public:
    TestEventConverterProcessEvent() = default;
};

TEST_F( // NOLINT
    TestEventConverterProcessEvent,
    testCreateProcessEventFromStringCanCreateProcessEventObjectWithExpectedValues)
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    ProcessEvent eventExpected = createDefaultProcessEvent();

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToProcessEvent(data.second);

    EXPECT_PRED_FORMAT2(processEventIsEquivalent, eventExpected, eventActual);
}

TEST_F( // NOLINT
    TestEventConverterProcessEvent,
    testCreateProcessEventFromStringCanCreateProcessEventObjectWithExpectedNonLatinCharacterValues)
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    ProcessEvent eventExpected = createDefaultProcessEvent();
    eventExpected.setCmdLine("いいい");

    std::pair<std::string, std::string> data = converter->eventToString(&eventExpected);

    auto eventActual = converter->stringToProcessEvent(data.second);

    EXPECT_PRED_FORMAT2(processEventIsEquivalent, eventExpected, eventActual);
}

TEST_F(TestEventConverterProcessEvent, testCreateProcessEventFromStringThrowsIfDataInvalidCapnString) // NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    EXPECT_THROW( // NOLINT
        converter->stringToProcessEvent("Not Valid Capn String"),
        Common::EventTypes::IEventException); // NOLINT
}

TEST_F(TestEventConverterProcessEvent, testCreateProcessEventFromStringThrowsIfDataTypeStringIsEmpty) // NOLINT
{
    std::unique_ptr<Common::EventTypes::IEventConverter> converter = Common::EventTypes::constructEventConverter();
    EXPECT_THROW(converter->stringToProcessEvent(""), Common::EventTypes::IEventException); // NOLINT
}

TEST_F(TestEventConverterProcessEvent, testCreateProcessEventForStartProcess) // NOLINT
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
    event.setSid("1001");

    Common::EventTypes::UserSid userSid;
    userSid.username = "testUser";
    userSid.sid = "1001";
    userSid.domain = "testDomain";
    event.setOwnerUserSid(userSid);

    Common::EventTypes::Pathname pathname;
    pathname.flags = 12;
    pathname.fileSystemType = 452;
    pathname.driveLetter = 6;
    pathname.pathname = "/name/of/path.tst";

    Common::EventTypes::TextOffsetLength openName;
    openName.length = 17;
    openName.offset = 0;
    pathname.openName = openName;

    Common::EventTypes::TextOffsetLength volumeName;
    volumeName.length = 12; // Not used on Linux dummy values set here to test de/serialisation
    volumeName.offset = 11; // Not used on Linux dummy values set here to test de/serialisation
    pathname.volumeName = volumeName;

    Common::EventTypes::TextOffsetLength shareName;
    shareName.length = 23; // Not used on Linux dummy values set here to test de/serialisation
    shareName.offset = 24; // Not used on Linux dummy values set here to test de/serialisation
    pathname.shareName = shareName;

    Common::EventTypes::TextOffsetLength extensionName;
    extensionName.length = 3;
    extensionName.offset = 14;
    pathname.extensionName = extensionName;

    Common::EventTypes::TextOffsetLength streamName;
    streamName.length = 10; // Not used on Linux dummy values set here to test de/serialisation
    streamName.offset = 9;  // Not used on Linux dummy values set here to test de/serialisation
    pathname.streamName = streamName;

    Common::EventTypes::TextOffsetLength finalComponentName;
    finalComponentName.length = 10;
    finalComponentName.offset = 8;
    pathname.finalComponentName = finalComponentName;

    Common::EventTypes::TextOffsetLength parentDirName;
    parentDirName.length = 9;
    parentDirName.offset = 0;
    pathname.parentDirName = parentDirName;

    event.setPathname(pathname);

    event.setCmdLine("example cmd line");

    event.setSha256("somesha256");
    event.setSha1("somesha1");
    event.setProcTitle("some proctitle");

    std::pair<std::string, std::string> data = converter->eventToString(&event);

    auto eventActual = converter->stringToProcessEvent(data.second);

    EXPECT_PRED_FORMAT2(processEventIsEquivalent, event, eventActual);
}

void testPathName(const std::string& parentDir, const std::string& filename, const std::string& extension)
{
    std::string pathString = parentDir + filename + (extension.empty() ? "" : ".") + extension;
    ProcessEvent event;
    event.setPathname(pathString);
    Common::EventTypes::Pathname pathname = event.getPathname();

    EXPECT_EQ(pathname.pathname.str(), pathString);

    auto openName = TextOffsetLength{ static_cast<uint32_t>(pathString.size()), 0 };
    EXPECT_EQ(pathname.openName.length, openName.length);
    EXPECT_EQ(pathname.openName.offset, openName.offset);

    auto parentDirName = TextOffsetLength{ static_cast<uint32_t>(parentDir.size()), 0 };
    EXPECT_EQ(pathname.parentDirName.length, parentDirName.length);
    EXPECT_EQ(pathname.parentDirName.offset, parentDirName.offset);

    auto finalComponentName = TextOffsetLength{
        static_cast<uint32_t>(filename.size() + extension.size() + (extension.empty() ? 0 : 1)),
        static_cast<uint32_t>(parentDir.empty() || (filename.empty() && extension.empty()) ? 0 : parentDir.size())
    };
    auto empty = TextOffsetLength{ 0, 0 };

    if (parentDir.empty())
    {
        finalComponentName = empty;
    }

    EXPECT_EQ(pathname.finalComponentName.length, finalComponentName.length);
    EXPECT_EQ(pathname.finalComponentName.offset, finalComponentName.offset);

    auto extensionName =
        TextOffsetLength{ static_cast<uint32_t>(extension.size()),
                          static_cast<uint32_t>(extension.empty() ? 0 : parentDir.size() + filename.size() + 1) };

    EXPECT_EQ(pathname.extensionName.length, extensionName.length);
    EXPECT_EQ(pathname.extensionName.offset, extensionName.offset);

    EXPECT_EQ(pathname.streamName.length, empty.length);
    EXPECT_EQ(pathname.streamName.offset, empty.offset);
    EXPECT_EQ(pathname.shareName.length, empty.length);
    EXPECT_EQ(pathname.shareName.offset, empty.offset);
    EXPECT_EQ(pathname.volumeName.length, empty.length);
    EXPECT_EQ(pathname.volumeName.offset, empty.offset);
    EXPECT_EQ(pathname.driveLetter, 0);
    EXPECT_EQ(pathname.fileSystemType, 0);
    EXPECT_EQ(pathname.flags, 0);
}

TEST_F(TestEventConverterProcessEvent, testPathnameCreationFromAString) // NOLINT
{
    testPathName("", "", "");
    testPathName("/", "", "");
    testPathName("/", "file", "");
    testPathName("/", "file", "sh");
    testPathName("/", "", "sh");
    testPathName("/this/is/a/", "", "");
    testPathName("/this/is/a/", "file", "");
    testPathName("/this/is/a/", "file", "sh");
    testPathName("/this/is/a/", "", "sh");
    testPathName("/this/is/a/file/", "", "");
    testPathName("./", "", "");
    testPathName("./", "file", "sh");
    testPathName("./", "file", "");
    testPathName("./", "", "sh");
    testPathName("./th.is/.is/a./fi.le/", "", "");
    testPathName("./th.is/.is/a./", "fi", "le");
    testPathName("./th.is/.is/a./", "fi.le", "sh");
}
