/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "modules/Common/FileSystem/IFileSystem.h"
#include "modules/Common/OSUtilitiesImpl/SXLMachineID.h"
#include "modules/Common/ProcessImpl/ArgcAndEnv.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
class TestSXLMachineID : public ::testing::Test
{
public:
    TestSXLMachineID()
    {
        mockIFileSystemPtr = new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockIFileSystemPtr));        
    }
    MockFileSystem* mockIFileSystemPtr;
    Tests::ScopedReplaceFileSystem m_replacer; 
    
};

TEST_F(TestSXLMachineID, SXLMachineIDShouldCreateMachineIDWhenInstalling) // NOLINT
{
    using ::testing::Invoke;
    Common::ProcessImpl::ArgcAndEnv argcAndEnv("notused", { { "rootinstall" } }, {});
    EXPECT_CALL(*mockIFileSystemPtr, isDirectory("rootinstall")).WillOnce(Return(true));
    EXPECT_CALL(*mockIFileSystemPtr, isFile("rootinstall/base/etc/machine_id.txt")).WillOnce(Return(false));
    std::string md5;
    EXPECT_CALL(*mockIFileSystemPtr, writeFile("rootinstall/base/etc/machine_id.txt", _))
        .WillOnce(Invoke([&md5](const std::string& /*path*/, const std::string& content) { md5 = content; }));

    Common::OSUtilitiesImpl::mainEntry(2, argcAndEnv.argc());
    EXPECT_EQ(md5.size(), 32);
}

TEST_F(TestSXLMachineID, SecondTimeItShouldNotRecreateMachineID) // NOLINT
{
    using ::testing::Invoke;
    Common::ProcessImpl::ArgcAndEnv argcAndEnv("notused", { { "rootinstall" } }, {});
    EXPECT_CALL(*mockIFileSystemPtr, isDirectory("rootinstall")).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockIFileSystemPtr, isFile("rootinstall/base/etc/machine_id.txt"))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    std::string md5;
    EXPECT_CALL(*mockIFileSystemPtr, writeFile("rootinstall/base/etc/machine_id.txt", _))
        .Times(1)
        .WillOnce(Invoke([&md5](const std::string& /*path*/, const std::string& content) { md5 = content; }));
    EXPECT_CALL(*mockIFileSystemPtr, readFile("rootinstall/base/etc/machine_id.txt"))
        .Times(1)
        .WillOnce(Return("ce9107bda89e91c8f277ace9056e1322"));

    Common::OSUtilitiesImpl::mainEntry(2, argcAndEnv.argc());
    EXPECT_EQ(md5.size(), 32);

    Common::OSUtilitiesImpl::mainEntry(2, argcAndEnv.argc());
}
