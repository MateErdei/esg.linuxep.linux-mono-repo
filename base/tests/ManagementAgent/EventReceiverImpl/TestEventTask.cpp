/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/EventReceiverImpl/EventTask.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TestHelpers/FileSystemReplaceAndRestore.h>
#include <Common/TestHelpers/MockFileSystem.h>

#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

class TestEventTask : public ::testing::Test
{

public:

    TestEventTask()
            : m_loggingSetup(new TestLogging::TestConsoleLoggingSetup())
    {

    }

private:
    TestLogging::TestConsoleLoggingSetupPtr m_loggingSetup;

};


TEST_F(TestEventTask, Construction) //NOLINT
{
    EXPECT_NO_THROW( //NOLINT
    ManagementAgent::EventReceiverImpl::EventTask task
            (
                    "APPID",
                    "EventXml"
            )
        );
}

StrictMock<MockFileSystem>* createMockFileSystem()
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    return filesystemMock;
}

TEST_F(TestEventTask, RunningATaskCausesAFileToBeCreated) //NOLINT
{
    ManagementAgent::EventReceiverImpl::EventTask task
            (
                    "APPID",
                    "EventXml"
            );


    auto filesystemMock = createMockFileSystem();

    EXPECT_CALL(*filesystemMock,
                writeFileAtomically(
                        MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml")
                        , "EventXml", "/opt/sophos-spl/tmp"
                )
            ).WillOnce(Return());

    Common::TestHelpers::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    task.run();
    Common::TestHelpers::restoreFileSystem();
}

TEST_F(TestEventTask, RunningTwoIdenticalTasksResultsInTwoDifferentFilesBeingCreated) //NOLINT
{

    ManagementAgent::EventReceiverImpl::EventTask task
            (
                    "APPID",
                    "EventXml"
            );

    ManagementAgent::EventReceiverImpl::EventTask task2
            (
                    "APPID",
                    "EventXml"
            );


    auto filesystemMock = createMockFileSystem();

    std::string base1;
    std::string base2;

    {
        InSequence seq;
        EXPECT_CALL(*filesystemMock,
                    writeFileAtomically(
                            MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"),
                            "EventXml",
                            "/opt/sophos-spl/tmp"
                    )).WillOnce(SaveArg<0>(&base1)
            );
        EXPECT_CALL(*filesystemMock,
                    writeFileAtomically(
                            MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"),
                            "EventXml",
                            "/opt/sophos-spl/tmp"
                    )).WillOnce(SaveArg<0>(&base2)
            );
    }
    Common::TestHelpers::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    task.run();
    task2.run();

    Common::TestHelpers::restoreFileSystem();

//    std::cerr << "base1=" << base1 << std::endl;
//    std::cerr << "base2=" << base2 << std::endl;
    EXPECT_NE(base1,base2);
}