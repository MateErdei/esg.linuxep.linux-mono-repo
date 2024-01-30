// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/EventReceiverImpl/EventTask.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <utility>

using namespace ManagementAgent::EventReceiverImpl;

namespace
{
    class TestEventTask : public ::testing::Test
    {
    public:
        TestEventTask() : m_loggingSetup(new Common::Logging::ConsoleLoggingSetup()) {}

        void SetUp() override
        {
        }

    private:
        std::unique_ptr<Common::Logging::ConsoleLoggingSetup> m_loggingSetup;
    };


}

TEST_F(TestEventTask, Construction)
{
    EXPECT_NO_THROW(
        ManagementAgent::EventReceiverImpl::EventTask task({"APPID", "EventXml"}));
}

namespace
{
    StrictMock<MockFileSystem>* createMockFileSystem()
    {
        auto filesystemMock = new StrictMock<MockFileSystem>();
        return filesystemMock;
    }
}

TEST_F(TestEventTask, RunningATaskCausesAFileToBeCreated)
{
    ManagementAgent::EventReceiverImpl::EventTask task({"APPID", "EventXml"});

    auto filesystemMock = createMockFileSystem();

    std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(
            MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "EventXml", tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    task.run();
}

TEST_F(TestEventTask, RunningTwoIdenticalTasksResultsInTwoDifferentFilesBeingCreated)
{
    ManagementAgent::EventReceiverImpl::EventTask task({"APPID", "EventXml"});

    ManagementAgent::EventReceiverImpl::EventTask task2({"APPID", "EventXml"});

    auto filesystemMock = createMockFileSystem();

    std::string base1;
    std::string base2;
    std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();

    {
        InSequence seq;
        EXPECT_CALL(
            *filesystemMock,
            writeFileAtomically(
                MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "EventXml", tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
            .WillOnce(SaveArg<0>(&base1));
        EXPECT_CALL(
            *filesystemMock,
            writeFileAtomically(
                MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "EventXml", tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
            .WillOnce(SaveArg<0>(&base2));
    }
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    task.run();
    task2.run();

    //    std::cerr << "base1=" << base1 << std::endl;
    //    std::cerr << "base2=" << base2 << std::endl;
    EXPECT_NE(base1, base2);
}