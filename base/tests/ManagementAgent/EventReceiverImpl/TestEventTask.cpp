/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/EventReceiverImpl/EventTask.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

class TestEventTask : public ::testing::Test
{
public:
    TestEventTask() : m_loggingSetup(new Common::Logging::ConsoleLoggingSetup()) {}

private:
    std::unique_ptr<Common::Logging::ConsoleLoggingSetup> m_loggingSetup;
};

TEST_F(TestEventTask, Construction) // NOLINT
{
    EXPECT_NO_THROW( // NOLINT
        ManagementAgent::EventReceiverImpl::EventTask task("APPID", "EventXml"));
}

StrictMock<MockFileSystem>* createMockFileSystem()
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    return filesystemMock;
}

TEST_F(TestEventTask, RunningATaskCausesAFileToBeCreated) // NOLINT
{
    ManagementAgent::EventReceiverImpl::EventTask task("APPID", "EventXml");

    auto filesystemMock = createMockFileSystem();

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(
            MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "EventXml", "/opt/sophos-spl/tmp", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    task.run();
}

TEST_F(TestEventTask, RunningTwoIdenticalTasksResultsInTwoDifferentFilesBeingCreated) // NOLINT
{
    ManagementAgent::EventReceiverImpl::EventTask task("APPID", "EventXml");

    ManagementAgent::EventReceiverImpl::EventTask task2("APPID", "EventXml");

    auto filesystemMock = createMockFileSystem();

    std::string base1;
    std::string base2;

    {
        InSequence seq;
        EXPECT_CALL(
            *filesystemMock,
            writeFileAtomically(
                MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "EventXml", "/opt/sophos-spl/tmp", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
            .WillOnce(SaveArg<0>(&base1));
        EXPECT_CALL(
            *filesystemMock,
            writeFileAtomically(
                MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "EventXml", "/opt/sophos-spl/tmp", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
            .WillOnce(SaveArg<0>(&base2));
    }
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    task.run();
    task2.run();

    //    std::cerr << "base1=" << base1 << std::endl;
    //    std::cerr << "base2=" << base2 << std::endl;
    EXPECT_NE(base1, base2);
}