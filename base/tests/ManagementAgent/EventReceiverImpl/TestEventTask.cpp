/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/EventReceiverImpl/EventTask.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <modules/Common/FileSystemImpl/FileSystemImpl.h>

TEST(TestEventTask, Construction) //NOLINT
{
    EXPECT_NO_THROW(
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
    EXPECT_CALL(*filesystemMock, join(_,_)).WillRepeatedly(Invoke([](const std::string& a, const std::string&b){return a + "/" + b; }));

    return filesystemMock;
}

TEST(TestEventTask, RunningATaskCausesAFileToBeCreated) //NOLINT
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

    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    task.run();
    Common::FileSystem::restoreFileSystem();
}

TEST(TestEventTask, RunningTwoIdenticalTasksResultsInTwoDifferentFilesBeingCreated) //NOLINT
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
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    task.run();
    task2.run();

    Common::FileSystem::restoreFileSystem();

//    std::cerr << "base1=" << base1 << std::endl;
//    std::cerr << "base2=" << base2 << std::endl;
    EXPECT_NE(base1,base2);
}