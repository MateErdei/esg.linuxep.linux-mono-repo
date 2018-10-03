/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilitiesImpl/SXLMachineID.h>
#include <Common/ProcessImpl/ArgcAndEnv.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
class TestSXLMachineID : public ::testing::Test
{
public:

    TestSXLMachineID()
    {
        std::unique_ptr<MockFileSystem> mockfileSystem (new StrictMock<MockFileSystem>());
        mockIFileSystemPtr = mockfileSystem.get();
        Common::FileSystem::replaceFileSystem(std::move(mockfileSystem));
    }
    ~TestSXLMachineID()
    {
        Common::FileSystem::restoreFileSystem();
    }
    MockFileSystem *  mockIFileSystemPtr;

};

TEST_F(TestSXLMachineID, SXLMachineIDShouldCreateMachineIDWhenInstalling) // NOLINT
{
    using ::testing::Invoke;
    Common::ProcessImpl::ArgcAndEnv argcAndEnv("notused", {{"rootinstall"}}, {});
    EXPECT_CALL(*mockIFileSystemPtr, isDirectory("rootinstall")).WillOnce(Return(true));
    EXPECT_CALL(*mockIFileSystemPtr, isFile("rootinstall/base/etc/machine_id.txt")).WillOnce(Return(false));
    std::string md5;
    EXPECT_CALL(*mockIFileSystemPtr, writeFile("rootinstall/base/etc/machine_id.txt", _)).WillOnce(Invoke([&md5](const std::string & path, const std::string &content){ md5 = content; }));

    Common::OSUtilitiesImpl::mainEntry(2, argcAndEnv.argc());
    EXPECT_EQ(md5.size(), 32);
}

TEST_F(TestSXLMachineID, SecondTimeItShouldNotRecreateMachineID) // NOLINT
{
    using ::testing::Invoke;
    Common::ProcessImpl::ArgcAndEnv argcAndEnv("notused", {{"rootinstall"}}, {});
    EXPECT_CALL(*mockIFileSystemPtr, isDirectory("rootinstall")).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockIFileSystemPtr, isFile("rootinstall/base/etc/machine_id.txt")).WillOnce(Return(false)).WillOnce(Return(true));
    std::string md5;
    EXPECT_CALL(*mockIFileSystemPtr, writeFile("rootinstall/base/etc/machine_id.txt", _)).Times(1).WillOnce(Invoke([&md5](const std::string & path, const std::string &content){ md5 = content; }));
    EXPECT_CALL(*mockIFileSystemPtr, readFile("rootinstall/base/etc/machine_id.txt")).Times(1).WillOnce(Return("ce9107bda89e91c8f277ace9056e1322"));

    Common::OSUtilitiesImpl::mainEntry(2, argcAndEnv.argc());
    EXPECT_EQ(md5.size(), 32);

    Common::OSUtilitiesImpl::mainEntry(2, argcAndEnv.argc());
}



