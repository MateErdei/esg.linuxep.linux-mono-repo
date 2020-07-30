/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>

#include "avscanner/avscannerimpl/DeviceUtil.h"
#include "avscanner/avscannerimpl/SystemCallWrapperFactory.h"

#include <linux/magic.h>
#include <sys/statfs.h>

using namespace avscanner::avscannerimpl;

using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArgReferee;
using ::testing::StrictMock;
using ::testing::_;

enum deviceType
{
    FIXED,
    NETWORK,
    OPTICAL,
    REMOVABLE,
    SYSTEM
};

class MockSystemCallWrapper : public ISystemCallWrapper
{
public:
    MOCK_METHOD3(_ioctl, int(int __fd, unsigned long int __request, char* buffer));
    MOCK_METHOD2(_statfs, int(const char *__file, struct ::statfs *__buf));
    MOCK_METHOD3(_open, int(const char *__file, int __oflag, mode_t mode));
};

class MockSystemCallWrapperFactory : public ISystemCallWrapperFactory
{
public:
    MOCK_METHOD0(createSystemCallWrapper, std::shared_ptr<ISystemCallWrapper>());
};

class TestDeviceUtil : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_systemCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        m_systemCallWrapperFactory = std::make_shared<StrictMock<MockSystemCallWrapperFactory>>();
        EXPECT_CALL(*m_systemCallWrapperFactory, createSystemCallWrapper()).WillOnce(Return(m_systemCallWrapper));
        m_deviceUtil = std::make_shared<DeviceUtil>(m_systemCallWrapperFactory);
    }

    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_systemCallWrapper;
    std::shared_ptr<StrictMock<MockSystemCallWrapperFactory>> m_systemCallWrapperFactory;
    std::shared_ptr<DeviceUtil> m_deviceUtil;
};

TEST_F(TestDeviceUtil, TestIsNetwork_UnknownType) // NOLINT
{
    EXPECT_TRUE(m_deviceUtil->isNetwork("1.2.3.4:/networkshare", "/mnt/bar", "unknown"));
    EXPECT_FALSE(m_deviceUtil->isNetwork("../dev/foo", "/mnt/bar", "unknown"));
    EXPECT_FALSE(m_deviceUtil->isNetwork("/dev/foo", "/mnt/bar", "unknown"));
}

TEST_F(TestDeviceUtil, TestIsSystem_UnknownType) // NOLINT
{
    EXPECT_FALSE(m_deviceUtil->isSystem("/dev/foo", "/mnt/bar", "unknown"));
}

TEST_F(TestDeviceUtil, TestIsFloppy_FloppyExistsAndHasHardware) // NOLINT
{
    std::string devicePath = "/dev/sdb";
    int fileDescriptor = 123;
    char driveType[] = "floppy";

    EXPECT_CALL(*m_systemCallWrapper, _open(devicePath.c_str(), O_RDONLY | O_NONBLOCK, 0644)).WillOnce(Return(fileDescriptor));
    EXPECT_CALL(*m_systemCallWrapper, _ioctl(fileDescriptor, _, _)).WillOnce(DoAll(SetArgPointee<2>(*driveType), Return(1)));

    EXPECT_TRUE(m_deviceUtil->isFloppy(devicePath, "/mnt/floppy", ""));
}

TEST_F(TestDeviceUtil, TestIsFloppy_FloppyDoesNotExist) // NOLINT
{
    std::string devicePath = "/dev/sdb";
    int fileDescriptor = -1;

    EXPECT_CALL(*m_systemCallWrapper, _open(devicePath.c_str(), O_RDONLY | O_NONBLOCK, 0644)).WillOnce(Return(fileDescriptor));

    EXPECT_FALSE(m_deviceUtil->isFloppy(devicePath, "/mnt/floppy", ""));
}

TEST_F(TestDeviceUtil, TestIsFloppy_FloppyExistsButHardwareDoesNot) // NOLINT
{
    std::string devicePath = "/dev/sdb";
    int fileDescriptor = 123;
    char driveType[] = "(null)";

    std::shared_ptr<StrictMock<MockSystemCallWrapper>> systemCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    EXPECT_CALL(*m_systemCallWrapper, _open(devicePath.c_str(), O_RDONLY | O_NONBLOCK, 0644)).WillOnce(Return(fileDescriptor));
    EXPECT_CALL(*m_systemCallWrapper, _ioctl(fileDescriptor, _, _)).WillOnce(DoAll(SetArgPointee<2>(*driveType), Return(1)));

    EXPECT_TRUE(m_deviceUtil->isFloppy(devicePath, "/mnt/floppy", ""));
}

TEST_F(TestDeviceUtil, TestIsSystem_noTypeButSpecialMount) // NOLINT
{
    std::shared_ptr<DeviceUtil> deviceUtil = std::make_shared<DeviceUtil>(std::make_shared<SystemCallWrapperFactory>());
    // Assumes all build machines will have /proc
    EXPECT_TRUE(deviceUtil->isSystem("proc", "/proc", "none"));
    EXPECT_TRUE(deviceUtil->isSystem("proc", "/proc", ""));
}

class DeviceUtilParameterizedTest
    : public ::testing::TestWithParam<std::tuple<std::string, deviceType>>
{
public:
    void SetUp() override
    {
        m_systemCallWrapper = std::make_shared<MockSystemCallWrapper>();
        m_systemCallWrapperFactory = std::make_shared<MockSystemCallWrapperFactory>();
        EXPECT_CALL(*m_systemCallWrapperFactory, createSystemCallWrapper()).WillOnce(Return(m_systemCallWrapper));
        m_deviceUtil = std::make_shared<DeviceUtil>(m_systemCallWrapperFactory);
    }

    std::shared_ptr<MockSystemCallWrapper> m_systemCallWrapper;
    std::shared_ptr<MockSystemCallWrapperFactory> m_systemCallWrapperFactory;
    std::shared_ptr<DeviceUtil> m_deviceUtil;
};

INSTANTIATE_TEST_CASE_P(TestDeviceUtil, DeviceUtilParameterizedTest, ::testing::Values(
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

    int fileDescriptor = -1;
    EXPECT_CALL(*m_systemCallWrapper, _open(devicePath.c_str(), O_RDONLY | O_NONBLOCK, 0644)).WillOnce(Return(fileDescriptor));

    EXPECT_EQ(m_deviceUtil->isNetwork(devicePath, mountPoint, filesystemType), expectedDeviceType == NETWORK);
    EXPECT_EQ(m_deviceUtil->isSystem(devicePath, mountPoint, filesystemType), expectedDeviceType == SYSTEM);
    EXPECT_EQ(m_deviceUtil->isOptical(devicePath, mountPoint, filesystemType), expectedDeviceType == OPTICAL);
    EXPECT_EQ(m_deviceUtil->isRemovable(devicePath, mountPoint, filesystemType), expectedDeviceType == OPTICAL);
}

class SpecialMountParameterizedTest
    : public ::testing::TestWithParam<uint64_t>
{
public:
    void SetUp() override
    {
        m_systemCallWrapper = std::make_shared<MockSystemCallWrapper>();
        m_systemCallWrapperFactory = std::make_shared<MockSystemCallWrapperFactory>();
        EXPECT_CALL(*m_systemCallWrapperFactory, createSystemCallWrapper()).WillOnce(Return(m_systemCallWrapper));
        m_deviceUtil = std::make_shared<DeviceUtil>(m_systemCallWrapperFactory);
    }

    std::shared_ptr<MockSystemCallWrapper> m_systemCallWrapper;
    std::shared_ptr<MockSystemCallWrapperFactory> m_systemCallWrapperFactory;
    std::shared_ptr<DeviceUtil> m_deviceUtil;
};

INSTANTIATE_TEST_CASE_P(TestDeviceUtil, SpecialMountParameterizedTest, ::testing::Values(
    PROC_SUPER_MAGIC,
    SYSFS_MAGIC,
    DEBUGFS_MAGIC,
    SECURITYFS_MAGIC,
    SELINUX_MAGIC,
    USBDEVICE_SUPER_MAGIC,
    DEVPTS_SUPER_MAGIC,
    0x62656570, // configfs
    0x65735543, // fusectl
    BINFMTFS_MAGIC,
    SOCKFS_MAGIC,
    PIPEFS_MAGIC,
    0x6e667364, // nfsd
    0x19800202, // mqueue
    CGROUP_SUPER_MAGIC,
    0x63677270, // CGROUP2_SUPER_MAGIC,
    0x67596969, // rpc_pipefs
    HUGETLBFS_MAGIC,
    0x1373, // devfs
    0x74726163 // TRACEFS_MAGIC
)); // NOLINT

TEST_P(SpecialMountParameterizedTest, TestIsSystem_noTypeButSpecialMount) // NOLINT
{
    std::string devicePath = "/dev/abc";
    std::string mountPoint = "/mnt/special";
    struct statfs sfs;
    sfs.f_type = GetParam();

    EXPECT_CALL(*m_systemCallWrapper, _statfs(mountPoint.c_str(), _)).WillRepeatedly(DoAll(SetArgPointee<1>(sfs), Return(0)));

    EXPECT_TRUE(m_deviceUtil->isSystem(devicePath, mountPoint, "none"));
    EXPECT_TRUE(m_deviceUtil->isSystem(devicePath, mountPoint, ""));
}
