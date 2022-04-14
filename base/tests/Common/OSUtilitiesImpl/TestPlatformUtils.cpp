/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/OSUtilitiesImpl/PlatformUtils.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(TestPlatformUtils, PopulateVendorDetailsForUbuntu) // NOLINT
{
    std::vector<std::string> etcIssueContents = { "Ubuntu 18.04.6 LTS \n" };
    std::vector<std::string> contents = {"DISTRIB_ID=Ubuntu", "DISTRIB_RELEASE=18.04", "DISTRIB_CODENAME=bionic", "DISTRIB_DESCRIPTION=\"Ubuntu 18.04.6 LTS\""};

    auto *filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));


    EXPECT_CALL(*filesystemMock, isFile("/etc/lsb-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/lsb-release")).WillRepeatedly(Return(contents));

    Common::OSUtilitiesImpl::PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "ubuntu");
    ASSERT_EQ(platformUtils.getOsName(), "Ubuntu 18.04.6 LTS");
    ASSERT_EQ(platformUtils.getOsMajorVersion(), "18");
    ASSERT_EQ(platformUtils.getOsMinorVersion(), "04");

    scopedReplaceFileSystem.reset();
}

TEST(TestPlatformUtils, PopulateVendorDetailsForRedhat) // NOLINT
{
    auto *filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> fileContents = {"Red Hat Enterprise Linux release"};
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, isFile("/etc/lsb-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/issue")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/centos-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/oracle-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/redhat-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/redhat-release")).WillRepeatedly(Return(fileContents));

    Common::OSUtilitiesImpl::PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "redhat");

    scopedReplaceFileSystem.reset();
}

TEST(TestPlatformUtils, PopulateVendorDetailsForCentos) // NOLINT
{
    auto *filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> fileContents = {"CentOS release"};
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, isFile("/etc/lsb-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/issue")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/centos-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/centos-release")).WillRepeatedly(Return(fileContents));

    Common::OSUtilitiesImpl::PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "centos");

    scopedReplaceFileSystem.reset();
}

TEST(TestPlatformUtils, PopulateVendorDetailsForAmazonLinux) // NOLINT
{
    auto *filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> fileContents = {"Amazon Linux release"};
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, isFile("/etc/lsb-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/issue")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/centos-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/oracle-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/redhat-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/system-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/system-release")).WillRepeatedly(Return(fileContents));

    Common::OSUtilitiesImpl::PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "amazon");

    scopedReplaceFileSystem.reset();
}

TEST(TestPlatformUtils, PopulateVendorDetailsForOracle) // NOLINT
{
    auto *filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> fileContents = {"Oracle Linux Server release"};
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, isFile("/etc/lsb-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/issue")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/centos-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/oracle-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/oracle-release")).WillRepeatedly(Return(fileContents));

    Common::OSUtilitiesImpl::PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "oracle");

    scopedReplaceFileSystem.reset();
}

TEST(TestPlatformUtils, PopulateVendorDetailsForMiracleLinux) // NOLINT
{
    auto *filesystemMock = new StrictMock<MockFileSystem>();
    std::vector<std::string> fileContents = {"MIRACLE LINUX release"};
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, isFile("/etc/lsb-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/issue")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/centos-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/oracle-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/redhat-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/system-release")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/miraclelinux-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/miraclelinux-release")).WillRepeatedly(Return(fileContents));

    Common::OSUtilitiesImpl::PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "miracle");

    scopedReplaceFileSystem.reset();
}