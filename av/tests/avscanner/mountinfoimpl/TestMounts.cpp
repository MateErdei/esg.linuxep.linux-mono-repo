/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/mountinfoimpl/Mounts.h"

#include "tests/common/LogInitializedTests.h"

#include <gmock/gmock.h>

#include <fstream>

#include <sys/stat.h>

using namespace avscanner::mountinfo;
using namespace avscanner::mountinfoimpl;

using ::testing::Return;
using ::testing::StrictMock;

namespace
{
    class MockSystemPaths : public avscanner::mountinfo::ISystemPaths
    {
    public:
        MOCK_CONST_METHOD0(mountInfoFilePath, std::string());
        MOCK_CONST_METHOD0(cmdlineInfoFilePath, std::string());
        MOCK_CONST_METHOD0(findfsCmdPath, std::string());
        MOCK_CONST_METHOD0(mountCmdPath, std::string());
    };

    class TestMounts : public LogInitializedTests
    {
    public:
        void SetUp() override
        {
            m_systemPaths = std::make_shared<StrictMock<MockSystemPaths>>();
        }

        void CreateFile(const std::string& filePath, const std::string& contents, mode_t mode = S_IRUSR)
        {
            ::unlink(filePath.c_str());
            std::ofstream filestream;
            filestream.open(filePath, std::ios::trunc);
            filestream << contents;
            filestream.close();
            chmod(filePath.c_str(), mode);
        }

        std::shared_ptr<StrictMock<MockSystemPaths>> m_systemPaths;
        const std::string m_findfsCmdPath = "findfsCmd.sh";
        const std::string m_mountCmdPath = "mountCmd.sh";
    };
}

TEST_F(TestMounts, TestMountInfoFile_DoesNotExist) // NOLINT
{
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return("/tmp/nonexistent.txt"));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPaths), std::runtime_error);
}

TEST_F(TestMounts, Empty_Mount_Info_Path) // NOLINT
{
    std::string mountInfoFile;
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPaths), std::runtime_error);
}

TEST_F(TestMounts, Empty_cmdline_info_path) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    std::string cmdlineInfoFile;
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPaths), std::runtime_error);
}

TEST_F(TestMounts, TestMountInfoFile_Exists) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    CreateFile(mountInfoFile, "/dev/abc1 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_GT(allMountpoints.size(), 0);
}

TEST_F(TestMounts, TestMountPoint_rootDirNotInMountInfoFile) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    CreateFile(mountInfoFile, "tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=1020904k,mode=755 0 0\n");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1\n", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_NE(mountInfo->device("/"), "");
    EXPECT_EQ(mountInfo->device("/run"), "tmpfs");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 2);
}

TEST_F(TestMounts, TestMountPoint_rootfs_uuid) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "/dev/root / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1\n", S_IRWXU);
    CreateFile(m_mountCmdPath, "#!/bin/bash\nexit 77\n", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_rootfs_label) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=LABEL=rootLabel ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1\n", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillOnce(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_rootfs_noLabelOrUUID) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic ro quiet splash");

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(0);
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "rootfs"); // Failed to resolve rootfs
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_findfsAndMountNotExecutable) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\necho /dev/abc1", S_IRUSR);
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.", S_IRUSR);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4"); // Failed to resolve device
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}


TEST_F(TestMounts, TestMountPoint_findfs_fails) // NOLINT
{
    std::string mountInfoFile = "mountInfo.txt";
    std::string cmdlineInfoFile = "cmdlineInfo.txt";
    CreateFile(mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#!/bin/bash\nexit 77", S_IRWXU);
    CreateFile(m_mountCmdPath, "#!/bin/bash\necho mount: /dev/abc1 mounted on /.", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(1).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(1).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

