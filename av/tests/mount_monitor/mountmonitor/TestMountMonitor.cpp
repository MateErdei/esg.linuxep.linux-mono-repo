// Copyright 2022, Sophos Limited.  All rights reserved.

#include <gtest/gtest.h>

#include "MockSystemPaths.h"
#include "MockSystemPathsFactory.h"
#include "MountMonitorMemoryAppenderUsingTests.h"

#include "avscanner/avscannerimpl/MockMountPoint.h"
#include "common/ThreadRunner.h"
#include "common/WaitForEvent.h"
#include "datatypes/MockSysCalls.h"
#include "datatypes/SystemCallWrapper.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"
#include "mount_monitor/mountinfoimpl/SystemPaths.h"
#include "sophos_on_access_process/fanotifyhandler/MockFanotifyHandler.h"

#include <fstream>

using namespace mount_monitor::mount_monitor;
using namespace testing;

namespace
{
    class TestMountMonitor : public MountMonitorMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
            m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            m_mockFanotifyHandler = std::make_shared<NiceMock<MockFanotifyHandler>>();
            m_mockSysPathsFactory = std::make_shared<StrictMock<MockSystemPathsFactory>>();
            m_mockSysPaths = std::make_shared<StrictMock<MockSystemPaths>>();
            m_sysPaths = std::make_shared<mount_monitor::mountinfoimpl::SystemPaths>();

            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCallWrapper;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
        std::shared_ptr<NiceMock<MockFanotifyHandler>> m_mockFanotifyHandler;
        std::shared_ptr<StrictMock<MockSystemPathsFactory>> m_mockSysPathsFactory;
        std::shared_ptr<StrictMock<MockSystemPaths>> m_mockSysPaths;
        std::shared_ptr<mount_monitor::mountinfoimpl::SystemPaths> m_sysPaths;
        WaitForEvent m_serverWaitGuard;
        fs::path m_testDir;
    };
}

TEST_F(TestMountMonitor, TestGetAllMountpoints)
{
    fs::path filePath = m_testDir / "testProcMounts";
    std::ofstream fileHandle(filePath);
    fileHandle << "/dev/sda1 / ext4 rw,relatime,errors=remount-ro,data=ordered 0 0" << std::endl;
    fileHandle << "tmpfs /run tmpfs rw,nosuid,noexec,relatime,size=1020640k,mode=755 0 0" << std::endl;
    fileHandle << "//UK-FILER6.ENG.SOPHOS/LINUX /mnt/uk-filer6/linux cifs rw,nosuid,nodev,noexec" << std::endl;
    fileHandle.close();
    size_t origFileSize = fs::file_size(filePath);

    OnAccessMountConfig config;
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillRepeatedly(Return(m_mockSysPaths));
    EXPECT_CALL(*m_mockSysPaths, mountInfoFilePath()).WillRepeatedly(Return(filePath));
    MountMonitor mountMonitor(config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_EQ(mountMonitor.getAllMountpoints().size(), 3);

    std::ofstream fileHandle2(filePath, std::ios::app);
    fileHandle2 << "//UK-FILER5.PROD.SOPHOS/PRODRO /uk-filer5/prodro cifs rw,nosuid,nodev,noexec" << std::endl;
    fileHandle2.close();
    EXPECT_EQ(mountMonitor.getAllMountpoints().size(), 4);

    // Restore the file to the original size - ie. simulate filer5 being unmounted
    fs::resize_file(filePath, origFileSize);
    EXPECT_EQ(mountMonitor.getAllMountpoints().size(), 3);
}

TEST_F(TestMountMonitor, TestGetIncludedMountpoints)
{
    OnAccessMountConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    std::shared_ptr<StrictMock<MockMountPoint>> localFixedDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillOnce(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillOnce(Return("test"));

    std::shared_ptr<StrictMock<MockMountPoint>> networkDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillOnce(Return(true));
    EXPECT_CALL(*networkDevice, mountPoint()).WillOnce(Return("test"));

    std::shared_ptr<StrictMock<MockMountPoint>> opticalDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*opticalDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isOptical()).WillOnce(Return(true));
    EXPECT_CALL(*opticalDevice, mountPoint()).WillOnce(Return("test"));

    std::shared_ptr<StrictMock<MockMountPoint>> removableDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*removableDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isRemovable()).WillOnce(Return(true));
    EXPECT_CALL(*removableDevice, mountPoint()).WillOnce(Return("test"));

    std::shared_ptr<StrictMock<MockMountPoint>> specialDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*specialDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isSpecial()).WillOnce(Return(true));
    EXPECT_CALL(*specialDevice, mountPoint()).WillRepeatedly(Return("test"));

    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(removableDevice);
    allMountpoints.push_back(specialDevice);

    EXPECT_CALL(*specialDevice, mountPoint()).WillOnce(Return("testSpecialMountPoint"));

    MountMonitor mountMonitor(config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 4);
}

TEST_F(TestMountMonitor, TestSetExclusions)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* excludedMount = "/test/";
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back(excludedMount);

    OnAccessMountConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = false;
    config.m_scanOptical = false;
    config.m_scanRemovable = false;

    std::shared_ptr<StrictMock<MockMountPoint>> localFixedDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillRepeatedly(Return(true));
    EXPECT_CALL(*localFixedDevice, isNetwork()).WillRepeatedly(Return(false));
    EXPECT_CALL(*localFixedDevice, isOptical()).WillRepeatedly(Return(false));
    EXPECT_CALL(*localFixedDevice, isRemovable()).WillRepeatedly(Return(false));
    EXPECT_CALL(*localFixedDevice, isSpecial()).WillRepeatedly(Return(false));
    EXPECT_CALL(*localFixedDevice, isDirectory()).WillRepeatedly(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillRepeatedly(Return(excludedMount));

    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillOnce(Return(m_sysPaths));

    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);

    MountMonitor mountMonitor(config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 1);
    mountMonitor.updateConfig(exclusions, config.m_scanNetwork);
    std::stringstream logMsg;
    logMsg << "Mount point " << excludedMount << " matches an exclusion in the policy and will be excluded from the scan";
    waitForLog(logMsg.str());
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 0);
}

TEST_F(TestMountMonitor, TestMountsEvaluatedOnProcMountsChange)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    OnAccessMountConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    WaitForEvent clientWaitGuard;

    struct pollfd fds[2]{};
    fds[1].revents = POLLPRI;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(
            DoAll(
                InvokeWithoutArgs(&clientWaitGuard, &WaitForEvent::waitDefault),
                SetArrayArgument<0>(fds, fds+2),
                Return(1)
                ))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&m_serverWaitGuard, &WaitForEvent::onEventNoArgs),
            Return(-1)
            )
          );
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillRepeatedly(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    auto numMountPoints = mountMonitor->getIncludedMountpoints(mountMonitor->getAllMountpoints()).size();
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    std::stringstream logMsg1;
    logMsg1 << "Including " << numMountPoints << " mount points in on-access scanning";
    EXPECT_TRUE(waitForLog(logMsg1.str()));
    EXPECT_EQ(appenderCount(logMsg1.str()), 1);

    clientWaitGuard.onEventNoArgs(); // Will allow the first call to complete
    m_serverWaitGuard.wait(); // Waits for the second call to start

    EXPECT_EQ(appenderCount(logMsg1.str()), 2);
}

TEST_F(TestMountMonitor, TestMountsEvaluatedOnProcMountsChangeStopStart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    OnAccessMountConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    WaitForEvent clientWaitGuard;

    struct pollfd fds[2]{};
    fds[1].revents = POLLPRI;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(InvokeWithoutArgs(&clientWaitGuard, &WaitForEvent::waitDefault),
                        SetArrayArgument<0>(fds, fds+2), Return(1)))
        .WillRepeatedly(DoDefault());


    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillRepeatedly(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    auto numMountPoints = mountMonitor->getIncludedMountpoints(mountMonitor->getAllMountpoints()).size();
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    std::stringstream logMsg1;
    logMsg1 << "Including " << numMountPoints << " mount points in on-access scanning";
    EXPECT_TRUE(waitForLog(logMsg1.str()));

    clientWaitGuard.onEventNoArgs(); // Will allow the first call to complete

    EXPECT_TRUE(waitForLogMultiple(logMsg1.str(), 2));

    clientWaitGuard.clear();

    mountMonitorThread.requestStopIfNotStopped();

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(InvokeWithoutArgs(&clientWaitGuard, &WaitForEvent::waitDefault),
                        SetArrayArgument<0>(fds, fds+2), Return(1)))
        .WillRepeatedly(DoDefault());

    mountMonitorThread.startIfNotStarted();

    EXPECT_TRUE(waitForLogMultiple(logMsg1.str(), 3));

    clientWaitGuard.onEventNoArgs(); // Will allow the first call to complete

    EXPECT_TRUE(waitForLogMultiple(logMsg1.str(), 4));
}

TEST_F(TestMountMonitor, TestMonitorExitsUsingPipe)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    OnAccessMountConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillOnce(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    EXPECT_TRUE(waitForLog("Stopping monitoring of mounts"));
}

TEST_F(TestMountMonitor, TestMonitorLogsErrorIfMarkingFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    OnAccessMountConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillOnce(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    EXPECT_TRUE(waitForLog("Unable to mark fanotify for mount point "));
    EXPECT_TRUE(waitForLog("On Access Scanning disabled"));
    EXPECT_TRUE(waitForLog("Stopping monitoring of mounts"));
}