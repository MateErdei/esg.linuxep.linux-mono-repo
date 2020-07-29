/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/DeviceUtil.h"

using namespace avscanner::avscannerimpl;

enum deviceType
{
    FIXED,
    NETWORK,
    OPTICAL,
    REMOVABLE,
    SYSTEM
};

class DeviceUtilParameterizedTest
        : public ::testing::TestWithParam<std::tuple<std::string, deviceType>>{

};

INSTANTIATE_TEST_CASE_P(DeviceUtil, DeviceUtilParameterizedTest, ::testing::Values(
        std::make_tuple("nfs", NETWORK),
        std::make_tuple("cifs", NETWORK),
        std::make_tuple("smbfs", NETWORK),
        std::make_tuple("smb", NETWORK),
        std::make_tuple("9p", NETWORK),
        std::make_tuple("ncpfs", NETWORK),
        std::make_tuple("afs", NETWORK),
        std::make_tuple("coda", NETWORK),
        std::make_tuple("nfs4", NETWORK),
        std::make_tuple("nfs3", NETWORK),
        std::make_tuple("isofs", OPTICAL),
        std::make_tuple("udf", OPTICAL),
        std::make_tuple("iso9660", OPTICAL),
        std::make_tuple("hsfs", OPTICAL),
        std::make_tuple("binfmt_misc", SYSTEM),
        std::make_tuple("configfs", SYSTEM),
        std::make_tuple("debugfs", SYSTEM),
        std::make_tuple("devfs", SYSTEM),
        std::make_tuple("devpts", SYSTEM),
        std::make_tuple("nfsd", SYSTEM),
        std::make_tuple("proc", SYSTEM),
        std::make_tuple("securityfs", SYSTEM),
        std::make_tuple("selinuxfs", SYSTEM),
        std::make_tuple("mqueue", SYSTEM),
        std::make_tuple("cgroup", SYSTEM),
        std::make_tuple("cgroup2", SYSTEM),
        std::make_tuple("rpc_pipefs", SYSTEM),
        std::make_tuple("hugetlbfs", SYSTEM),
        std::make_tuple("sysfs", SYSTEM),
        std::make_tuple("fusectl", SYSTEM),
        std::make_tuple("pipefs", SYSTEM),
        std::make_tuple("sockfs", SYSTEM),
        std::make_tuple("usbfs", SYSTEM),
        std::make_tuple("tracefs", SYSTEM)
)); // NOLINT

TEST_P(DeviceUtilParameterizedTest, TestDeviceType) // NOLINT
{
    std::string devicePath = "/dev/foo";
    std::string mountPoint = "/mnt/bar";
    const std::string& filesystemType = std::get<0>(GetParam());
    deviceType expectedDeviceType = std::get<1>(GetParam());

    EXPECT_EQ(DeviceUtil::isNetwork(devicePath, mountPoint, filesystemType), expectedDeviceType == NETWORK);
    EXPECT_EQ(DeviceUtil::isSystem(devicePath, mountPoint, filesystemType), expectedDeviceType == SYSTEM);
    EXPECT_EQ(DeviceUtil::isOptical(devicePath, mountPoint, filesystemType), expectedDeviceType == OPTICAL);
    EXPECT_EQ(DeviceUtil::isRemovable(devicePath, mountPoint, filesystemType), expectedDeviceType == OPTICAL);
}

TEST(DeviceUtil, TestIsNetwork_UnknownType) // NOLINT
{
    EXPECT_TRUE(DeviceUtil::isNetwork("1.2.3.4:/networkshare", "/mnt/bar", "unknown"));
    EXPECT_FALSE(DeviceUtil::isNetwork("../dev/foo", "/mnt/bar", "unknown"));
    EXPECT_FALSE(DeviceUtil::isNetwork("/dev/foo", "/mnt/bar", "unknown"));
}

TEST(DeviceUtil, TestIsSystem_UnknownType) // NOLINT
{
    EXPECT_FALSE(DeviceUtil::isSystem("/dev/foo", "/mnt/bar", "unknown"));
}

TEST(DeviceUtil, TestIsSystem_noTypeButSpecialMount) // NOLINT
{
    // Assumes all build machines will have /proc
    EXPECT_TRUE(DeviceUtil::isSystem("proc", "/proc", "none"));
    EXPECT_TRUE(DeviceUtil::isSystem("proc", "/proc", ""));
}