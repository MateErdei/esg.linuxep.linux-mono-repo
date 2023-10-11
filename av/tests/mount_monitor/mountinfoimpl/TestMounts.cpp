// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "tests/common/LogInitializedTests.h"

#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPaths.h"
#include "datatypes/sophos_filesystem.h"

#include <gmock/gmock.h>

#include <fstream>

#include <sys/stat.h>

namespace fs = sophos_filesystem;

using namespace mount_monitor::mountinfo;
using namespace mount_monitor::mountinfoimpl;

using ::testing::Return;
using ::testing::StrictMock;

namespace
{
    class MockSystemPaths : public mount_monitor::mountinfo::ISystemPaths
    {
    public:
        MOCK_CONST_METHOD0(mountInfoFilePath, std::string());
        MOCK_CONST_METHOD0(cmdlineInfoFilePath, std::string());
        MOCK_CONST_METHOD0(findfsCmdPath, std::string());
        MOCK_CONST_METHOD0(mountCmdPath, std::string());
    };

    class TestMounts : public LogInitializedTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            m_systemPaths = std::make_shared<StrictMock<MockSystemPaths>>();

            m_findfsCmdPath = m_testDir / "findfsCmd.sh";
            m_mountCmdPath = m_testDir / "mountCmd.sh";
            m_mountInfoFile = m_testDir / "mountInfo.txt";
            m_cmdlineInfoFile = m_testDir / "cmdlineInfo.txt";
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
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

        fs::path m_testDir;
        std::shared_ptr<StrictMock<MockSystemPaths>> m_systemPaths;
        std::string m_findfsCmdPath;
        std::string m_mountCmdPath;
        std::string m_mountInfoFile;
        std::string m_cmdlineInfoFile;
    };
}

TEST_F(TestMounts, TestMountInfoFile_DoesNotExist)
{
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_testDir / "nonexistent.txt"));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPaths), std::system_error);
}

TEST_F(TestMounts, Empty_Mount_Info_Path)
{
    std::string mountInfoFile;
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(mountInfoFile));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPaths), std::runtime_error);
}

TEST_F(TestMounts, Empty_cmdline_info_path)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    std::string cmdlineInfoFile;
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(cmdlineInfoFile));
    EXPECT_THROW(std::make_shared<Mounts>(m_systemPaths), std::runtime_error);
}

TEST_F(TestMounts, TestMountInfoFile_Exists)
{
    CreateFile(m_mountInfoFile, "/dev/abc1 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_GT(allMountpoints.size(), 0);
}

TEST_F(TestMounts, TestMountPoint_rootDirNotInMountInfoFile)
{
    CreateFile(m_mountInfoFile, "tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=1020904k,mode=755 0 0\n");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\necho /dev/abc1\n", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    std::shared_ptr<Mounts> mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_NE(mountInfo->device("/"), "");
    EXPECT_EQ(mountInfo->device("/run"), "tmpfs");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 2);
}

TEST_F(TestMounts, TestMountPoint_rootfs_uuid)
{
    CreateFile(m_mountInfoFile, "/dev/root / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\necho /dev/abc1\n", S_IRWXU);
    CreateFile(m_mountCmdPath, "#! /bin/sh\nexit 77\n", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_rootfs_label)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=LABEL=rootLabel ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\necho /dev/abc1\n", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillOnce(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_rootfs_noLabelOrUUID)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic ro quiet splash");

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(0);
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "rootfs"); // Failed to resolve rootfs
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, TestMountPoint_findfsAndMountNotExecutable)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\necho /dev/abc1", S_IRUSR);
    CreateFile(m_mountCmdPath, "#! /bin/sh\necho mount: /dev/abc1 mounted on /.", S_IRUSR);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4"); // Failed to resolve device
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}


TEST_F(TestMounts, TestMountPoint_findfs_fails)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\nexit 77", S_IRWXU);
    CreateFile(m_mountCmdPath, "#! /bin/sh\necho mount: /dev/abc1 mounted on /.", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(1).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(1).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}


TEST_F(TestMounts, TestMountPoint_findfs_fails_mount_returns_empty)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\nexit 77", S_IRWXU);
    CreateFile(m_mountCmdPath, "#! /bin/sh\nexit 0", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(1).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(1).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4"); // Failed to resolve to device
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}


TEST_F(TestMounts, TestMountPoint_findfs_fails_mount_returns_one_space)
{
    CreateFile(m_mountInfoFile, "rootfs / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n");
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\nexit 77", S_IRWXU);
    CreateFile(m_mountCmdPath, "#! /bin/sh\necho mount: foobar", S_IRWXU);

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(1).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(1).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4"); // Failed to resolve to device
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}

TEST_F(TestMounts, octalEscaped)
{
    CreateFile(m_mountInfoFile,
               "/dev/abc1 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n"
               "/dev/with\\040space /mnt xfs rw,relatime,errors=remount-ro,data=ordered 0 0\n"
               "/dev/ghi1 /with\\011tab xfs rw,relatime,errors=remount-ro,data=ordered 0 0\n"
               "/dev/digit\\1340follows /home xfs rw,relatime,errors=remount-ro,data=ordered 0 0\n"
               );

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    EXPECT_EQ(mountInfo->device("/mnt"), "/dev/with\040space");
    EXPECT_EQ(mountInfo->device("/with\011tab"), "/dev/ghi1");
    EXPECT_EQ(mountInfo->device("/home"), "/dev/digit\\0follows");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 4);
}

TEST_F(TestMounts, octalEscapedInvalid)
{
    CreateFile(m_mountInfoFile,
               "/dev/abc1 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0\n"
               "/dev/short\\01.code /mnt xfs rw,relatime,errors=remount-ro,data=ordered 0 0\n"
               "/dev/ghi1 /invalid\\018digit xfs rw,relatime,errors=remount-ro,data=ordered 0 0\n"
    );

    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(0);

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/abc1");
    EXPECT_EQ(mountInfo->device("/mnt"), "/dev/short\\01.code");
    EXPECT_EQ(mountInfo->device("/invalid\\018digit"), "/dev/ghi1");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 3);
}

TEST_F(TestMounts, getMountFromPath_doesNotIncorrectlyMatchToplevelFoldersWithMounts)
{
    auto systemPaths = std::make_shared<mount_monitor::mountinfoimpl::SystemPaths>();
    auto mountInfo = std::make_shared<Mounts>(systemPaths);
    auto rootMount = mountInfo->getMountFromPath("/procFolder/eicar.com");
    EXPECT_NE(rootMount->mountPoint(), "/proc");
    EXPECT_FALSE(rootMount->isSpecial());
}

TEST_F(TestMounts, getMountFromPath_maintainsOrderOfMounts)
{
    CreateFile(m_mountInfoFile,
               "rootfs / rootfs rw 0 0\n"
               "/dev/sda1 /a/b/c ext4 rw 0 0\n"
               "/dev/sda2 /a ext4 rw 0 0\n"
               "/dev/sda3 /a/b ext4 rw 0 0\n"
    );
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\nexit 77", S_IRWXU);
    CreateFile(m_mountCmdPath, "#! /bin/sh\necho mount: foobar", S_IRWXU);
    
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).Times(1).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(1).WillRepeatedly(Return(m_mountCmdPath));
    
    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    auto rootMount = mountInfo->getMountFromPath("/a/b/c/d");
    EXPECT_EQ(rootMount->device(), "/dev/sda3");
    EXPECT_EQ(rootMount->mountPoint(), "/a/b");
}

TEST_F(TestMounts, root_directory_is_a_directory)
{
    auto systemPaths = std::make_shared<mount_monitor::mountinfoimpl::SystemPaths>();
    auto mountInfo = std::make_shared<Mounts>(systemPaths);
    auto rootMount = mountInfo->getMountFromPath("/");
    EXPECT_TRUE(rootMount->isDirectory());
}

TEST_F(TestMounts, subsequentDuplicateMountPointReplacesPreviousEntry)
{
    CreateFile(m_mountInfoFile,
               "rootfs / rootfs rw 0 0\n"
               "/dev/mapper/centos-root / xfs rw,seclabel,relatime,attr2,inode64,noquota 0 0\n"
    );
    CreateFile(m_cmdlineInfoFile, "BOOT_IMAGE=/boot/vmlinuz-4.15.0-123-generic root=UUID=9232e4a0-bdf4-4c83-9d9c-68816a9809a4 ro quiet splash");
    CreateFile(m_findfsCmdPath, "#! /bin/sh\necho /dev/abc1", S_IRUSR);
    CreateFile(m_mountCmdPath, "#! /bin/sh\nexit 0", S_IRWXU);
    
    EXPECT_CALL(*m_systemPaths, mountInfoFilePath()).WillOnce(Return(m_mountInfoFile));
    EXPECT_CALL(*m_systemPaths, cmdlineInfoFilePath()).WillOnce(Return(m_cmdlineInfoFile));
    EXPECT_CALL(*m_systemPaths, findfsCmdPath()).WillRepeatedly(Return(m_findfsCmdPath));
    EXPECT_CALL(*m_systemPaths, mountCmdPath()).Times(1).WillRepeatedly(Return(m_mountCmdPath));

    auto mountInfo = std::make_shared<Mounts>(m_systemPaths);
    EXPECT_EQ(mountInfo->device("/"), "/dev/mapper/centos-root");
    auto allMountpoints = mountInfo->mountPoints();
    EXPECT_EQ(allMountpoints.size(), 1);
}
