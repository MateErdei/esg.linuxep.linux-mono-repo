/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <tests/common/LogInitializedTests.h>

#include "avscanner/avscannerimpl/Mounts.h"

#include "datatypes/sophos_filesystem.h"

#include <fstream>
#include <sys/stat.h>

namespace fs = sophos_filesystem;

using namespace avscanner::avscannerimpl;

using ::testing::Return;
using ::testing::StrictMock;

class MockSystemPaths : public ISystemPaths
{
public:
    MOCK_CONST_METHOD0(mountInfoFilePath, std::string());
    MOCK_CONST_METHOD0(cmdlineInfoFilePath, std::string());
    MOCK_CONST_METHOD0(findfsCmdPath, std::string());
    MOCK_CONST_METHOD0(mountCmdPath, std::string());
};

class MockSystemPathsFactory : public ISystemPathsFactory
{
public:
    MOCK_METHOD0(createSystemPaths, std::shared_ptr<ISystemPaths>());
};

class TestMounts : public LogInitializedTests
{
public:
    void SetUp() override
    {
        m_systemPaths = std::make_shared<StrictMock<MockSystemPaths>>();
        m_systemPathsFactory = std::make_shared<StrictMock<MockSystemPathsFactory>>();
        EXPECT_CALL(*m_systemPathsFactory, createSystemPaths).WillRepeatedly(Return(m_systemPaths));
        m_findfsCmdPath = "findfsCmd.sh";
        m_mountCmdPath = "mountCmd.sh";
    }

    void CreateFile(const std::string& filePath, const std::string& contents)
    {
        std::ofstream mountInfoFH;
        mountInfoFH.open (filePath);
        mountInfoFH << contents;
        mountInfoFH.close();
    }

    std::shared_ptr<StrictMock<MockSystemPaths>> m_systemPaths;
    std::shared_ptr<StrictMock<MockSystemPathsFactory>> m_systemPathsFactory;
    std::string m_findfsCmdPath;
    std::string m_mountCmdPath;
};

TEST_F(TestMounts, TestMountInfoFile_DoesNotExist) // NOLINT
{
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return("/tmp/nonexistent.txt"));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPathsFactory), std::runtime_error);
}

TEST_F(TestMounts, TestMountInfoFile_Exists) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    CreateFile(mountInfoFile, "/dev/abc1 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPathsFactory);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_GT(allMountpoints.size(), 0);
}

TEST_F(TestMounts, TestMountPoint_rootDirNotInMountInfoFile) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    CreateFile(mountInfoFile, "tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=1020904k,mode=755 0 0\n");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1");
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.");
    chmod(m_findfsCmdPath.c_str(), S_IRWXU);
    chmod(m_mountCmdPath.c_str(), S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPathsFactory);
    EXPECT_NE(mountInfo->device("/"), "");
    EXPECT_EQ(mountInfo->device("/run"), "tmpfs");
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 2);
}

TEST_F(TestMounts, TestMountPoint_rootfs_uuid) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "/dev/root / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1");
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.");
    chmod(m_findfsCmdPath.c_str(), S_IRWXU);
    chmod(m_mountCmdPath.c_str(), S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPathsFactory);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_rootfs_label) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=LABEL=rootLabel ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1");
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.");
    chmod(m_findfsCmdPath.c_str(), S_IRWXU);
    chmod(m_mountCmdPath.c_str(), S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPathsFactory);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_rootfs_noLabelOrUUID) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1");
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.");
    chmod(m_findfsCmdPath.c_str(), S_IRWXU);
    chmod(m_mountCmdPath.c_str(), S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPathsFactory);
    EXPECT_EQ(mountInfo->device("/"), "rootfs");
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_findfsAndMountNotExecutable) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1");
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.");
    chmod(m_findfsCmdPath.c_str(), S_IRUSR);
    chmod(m_mountCmdPath.c_str(), S_IRUSR);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    std::shared_ptr<IMountInfo> mountInfo = std::make_shared<Mounts>(m_systemPathsFactory);
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}