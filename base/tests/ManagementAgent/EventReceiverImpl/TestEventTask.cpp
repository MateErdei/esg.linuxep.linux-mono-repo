/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/EventReceiverImpl/EventTask.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>

TEST(TestEventTask, Construction) //NOLINT
{
    EXPECT_NO_THROW(
    ManagementAgent::EventReceiverImpl::EventTask task
            (
                    "mcsdir",
                    "APPID",
                    "EventXml"
            )
        );
}

TEST(TestEventTask, RunningTaskCreatesFile) //NOLINT
{
    ManagementAgent::EventReceiverImpl::EventTask task
            (
                    "mcsdir",
                    "APPID",
                    "EventXml"
            );


    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, join(_,_)).WillRepeatedly(Invoke([](const std::string& a, const std::string&b){return a + "/" + b; }));

    // NOLINT
    EXPECT_CALL(*filesystemMock, writeFileAtomically(
            ::testing::AllOf(StartsWith("mcsdir/event/APPID_event-"),EndsWith(".xml")),"EventXml","mcsdir/tmp")).WillOnce(Return()); // NOLINT

    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    task.run();
    Common::FileSystem::restoreFileSystem();
}

TEST(TestEventTask, 2EventsHaveDifferentFilenames) //NOLINT
{

    ManagementAgent::EventReceiverImpl::EventTask task
            (
                    "mcsdir",
                    "APPID",
                    "EventXml"
            );

    ManagementAgent::EventReceiverImpl::EventTask task2
            (
                    "mcsdir",
                    "APPID",
                    "EventXml"
            );


    auto filesystemMock = new MockFileSystem();

    std::string base1;
    std::string base2;

    EXPECT_CALL(*filesystemMock, join(_,_)).WillRepeatedly(Invoke([](const std::string& a, const std::string&b){return a + "/" + b; }));

    {
        InSequence seq;
        EXPECT_CALL(*filesystemMock,
                    writeFileAtomically(::testing::AllOf(StartsWith("mcsdir/event/APPID_event-"), EndsWith(".xml")), "EventXml",
                                        "mcsdir/tmp"
                    )).
                              WillOnce(
                SaveArg<0>(&base1)
        ); // NOLINT
        EXPECT_CALL(*filesystemMock,
                    writeFileAtomically(::testing::AllOf(StartsWith("mcsdir/event/APPID_event-"), EndsWith(".xml")), "EventXml",
                                        "mcsdir/tmp"
                    )).
                              WillOnce(
                SaveArg<0>(&base2)
        ); // NOLINT
    }
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    task.run();
    task2.run();

    Common::FileSystem::restoreFileSystem();

//    std::cerr << "base1=" << base1 << std::endl;
//    std::cerr << "base2=" << base2 << std::endl;
    EXPECT_NE(base1,base2);
}