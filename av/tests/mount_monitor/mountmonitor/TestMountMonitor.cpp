// Copyright 2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

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
#include "sophos_on_access_process/soapd_bootstrap/OnAccessProductConfigDefaults.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <fstream>

using namespace mount_monitor::mount_monitor;
using namespace mount_monitor::mountinfo;
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
        sophos_on_access_process::OnAccessConfig::OnAccessConfiguration m_config {};
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

    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillRepeatedly(Return(m_mockSysPaths));
    EXPECT_CALL(*m_mockSysPaths, mountInfoFilePath()).WillRepeatedly(Return(filePath));
    MountMonitor mountMonitor(m_config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_EQ(mountMonitor.getAllMountpoints().size(), 3);

    std::ofstream fileHandle2(filePath, std::ios::app);
    fileHandle2 << "//UK-FILER5.PROD.SOPHOS/PRODRO /uk-filer5/prodro cifs rw,nosuid,nodev,noexec" << std::endl;
    fileHandle2.close();
    EXPECT_EQ(mountMonitor.getAllMountpoints().size(), 4);

    // Restore the file to the original size - i.e. simulate filer5 being unmounted
    fs::resize_file(filePath, origFileSize);
    EXPECT_EQ(mountMonitor.getAllMountpoints().size(), 3);
}

TEST_F(TestMountMonitor, TestGetIncludedMountpoints)
{
    std::shared_ptr<NiceMock<MockMountPoint>> localFixedDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillOnce(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillOnce(Return("localFixedDevice"));

    std::shared_ptr<NiceMock<MockMountPoint>> networkDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillOnce(Return(true));
    EXPECT_CALL(*networkDevice, mountPoint()).WillOnce(Return("networkDevice"));

    std::shared_ptr<NiceMock<MockMountPoint>> removableDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*removableDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isRemovable()).WillOnce(Return(true));
    EXPECT_CALL(*removableDevice, mountPoint()).WillOnce(Return("removableDevice"));

    std::shared_ptr<NiceMock<MockMountPoint>> opticalDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*opticalDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isOptical()).WillOnce(Return(true));
    EXPECT_CALL(*opticalDevice, mountPoint()).WillOnce(Return("opticalDevice"));

    std::shared_ptr<NiceMock<MockMountPoint>> specialDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*specialDevice, isSpecial()).WillOnce(Return(true));
    EXPECT_CALL(*specialDevice, mountPoint()).WillRepeatedly(Return("specialDevice"));

    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(removableDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(specialDevice);

    EXPECT_CALL(*specialDevice, mountPoint()).WillOnce(Return("testSpecialMountPoint"));

    MountMonitor mountMonitor(m_config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 4);
}

TEST_F(TestMountMonitor, TestSetExclusions)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* excludedMount = "/test/";
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back(excludedMount);

    std::shared_ptr<NiceMock<MockMountPoint>> localFixedDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillRepeatedly(Return(true));
    EXPECT_CALL(*localFixedDevice, isDirectory()).WillRepeatedly(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillRepeatedly(Return(excludedMount));

    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);

    MountMonitor mountMonitor(m_config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 1);

    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));

    sophos_on_access_process::OnAccessConfig::OnAccessConfiguration config{};
    config.exclusions = exclusions;
    mountMonitor.updateConfig(config);

    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 0);
    std::stringstream logMsg;
    logMsg << "Mount point " << excludedMount << " matches an exclusion in the policy and will be excluded from the scan";
    EXPECT_TRUE(appenderContains(logMsg.str()));
}

TEST_F(TestMountMonitor, TestUpdateConfigSetsAllConfigBeforeReenumeratingMounts)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* excludedMount = "/test/";
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back(excludedMount);

    std::shared_ptr<NiceMock<MockMountPoint>> localFixedDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillRepeatedly(Return(true));
    EXPECT_CALL(*localFixedDevice, isDirectory()).WillRepeatedly(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillRepeatedly(Return(excludedMount));

    std::shared_ptr<NiceMock<MockMountPoint>> networkDevice = std::make_shared<NiceMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillRepeatedly(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillRepeatedly(Return(true));
    EXPECT_CALL(*networkDevice, isSpecial()).WillRepeatedly(Return(false));
    EXPECT_CALL(*networkDevice, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isDirectory()).WillRepeatedly(Return(true));
    EXPECT_CALL(*networkDevice, mountPoint()).WillRepeatedly(Return("network"));

    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillRepeatedly(Return(m_sysPaths));
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));

    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);

    MountMonitor mountMonitor(m_config, m_sysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 2);

    sophos_on_access_process::OnAccessConfig::OnAccessConfiguration config{};
    config.enabled = true;
    config.excludeRemoteFiles = true;
    config.exclusions = exclusions;
    mountMonitor.updateConfig(config);
    EXPECT_TRUE(appenderContains("OA config changed, re-enumerating mount points"));

    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 0);
    std::stringstream logMsg;
    logMsg << "Mount point " << excludedMount << " matches an exclusion in the policy and will be excluded from the scan";
    EXPECT_TRUE(appenderContains(logMsg.str()));
    EXPECT_TRUE(appenderContains("Mount point network has been excluded from scanning"));
}

TEST_F(TestMountMonitor, TestMountsEvaluatedOnProcMountsChange)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
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
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
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
    WaitForEvent clientWaitGuard;

    struct pollfd fds[2]{};
    fds[1].revents = POLLPRI;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(InvokeWithoutArgs(&clientWaitGuard, &WaitForEvent::waitDefault),
                        SetArrayArgument<0>(fds, fds+2), Return(1)))
        .WillRepeatedly(DoDefault());


    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillRepeatedly(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    auto numMountPoints = mountMonitor->getIncludedMountpoints(mountMonitor->getAllMountpoints()).size();
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    std::stringstream logMsg1;
    logMsg1 << "Including " << numMountPoints << " mount points in on-access scanning";
    EXPECT_TRUE(waitForLog(logMsg1.str()));

    clientWaitGuard.onEventNoArgs(); // Will allow the first call to complete

    EXPECT_TRUE(waitForLogMultiple(logMsg1.str(), 2, 250ms));

    clientWaitGuard.clear();

    mountMonitorThread.requestStopIfNotStopped();

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(InvokeWithoutArgs(&clientWaitGuard, &WaitForEvent::waitDefault),
                        SetArrayArgument<0>(fds, fds+2), Return(1)))
        .WillRepeatedly(DoDefault());

    mountMonitorThread.startIfNotStarted();

    EXPECT_TRUE(waitForLogMultiple(logMsg1.str(), 3, 250ms));

    clientWaitGuard.onEventNoArgs(); // Will allow the first call to complete

    EXPECT_TRUE(waitForLogMultiple(logMsg1.str(), 4, 250ms));
}

TEST_F(TestMountMonitor, TestMonitorExitsUsingPipe)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillOnce(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    EXPECT_TRUE(waitForLog("Stopping monitoring of mounts"));
}

TEST_F(TestMountMonitor, TestMonitorLogsErrorIfMarkingFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(-1));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillOnce(Return(m_sysPaths));
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    EXPECT_TRUE(waitForLog("Unable to mark fanotify for mount point "));
    EXPECT_TRUE(waitForLog("On Access Scanning disabled"));
    EXPECT_TRUE(waitForLog("Stopping monitoring of mounts"));
}

TEST_F(TestMountMonitor, TestMonitorLogsTelemetryFileSystemType)
{
    const std::string expectedFileSystem = "tmpfs";
    UsingMemoryAppender memoryAppenderHolder(*this);
    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockFanotifyHandler, markMount(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSysPathsFactory, createSystemPaths()).WillOnce(Return(m_sysPaths));

    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor", true);

    //Wait for us to finish processing mountpoints
    EXPECT_TRUE(waitForLog("mount points in on-access scanning"));

    auto resContent = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    PRINT(resContent);
    EXPECT_TRUE(resContent.find(expectedFileSystem) != std::string::npos);
    EXPECT_TRUE(waitForLog("Stopping monitoring of mounts"));
}

TEST_F(TestMountMonitor, TestMonitorFileSystemTelemetryIsPersistant)
{
    std::set<std::string> input {
        "squashfs",
        "tmpfs"
    };
    auto expectedFileSystemStr =R"sophos({"file-system-types":["squashfs","tmpfs"]})sophos";

    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    mountMonitor->addFileSystemToTelemetry(input);
    auto firstContent = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(firstContent, expectedFileSystemStr);

    auto secondContent = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(firstContent, secondContent);
}

TEST_F(TestMountMonitor, TestMonitorFileSystemTelemetryCanBeChangedAndNotDuplicated)
{
    std::set<std::string> input1 {
        "squashfs",
        "tmpfs"
    };

    std::set<std::string> input2 {
        "cifs",
        "ext4"
    };

    auto expectedFileSystemStr1 =R"sophos({"file-system-types":["squashfs","tmpfs"]})sophos";
    auto expectedFileSystemStr2 =R"sophos({"file-system-types":["cifs","ext4"]})sophos";

    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    mountMonitor->addFileSystemToTelemetry(input1);
    auto firstContent = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(firstContent, expectedFileSystemStr1);

    mountMonitor->addFileSystemToTelemetry(input2);
    auto secondContent = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(secondContent, expectedFileSystemStr2);
}

TEST_F(TestMountMonitor, TestMonitorFileSystemTelemetryDoesntIncludeSpecialMP)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto expectedFileSystemStr =R"sophos({"file-system-types":["ValidFileSystem"]})sophos";

    std::vector<IMountPointSharedPtr> mountPointVec;
    auto testSkipDevice = std::make_shared<NiceMock<MockMountPoint>>();
    auto testIncDevice = std::make_shared<NiceMock<MockMountPoint>>();
    mountPointVec.push_back(testSkipDevice);
    mountPointVec.push_back(testIncDevice);

    EXPECT_CALL(*testSkipDevice, isSpecial()).WillOnce(Return(true));

    EXPECT_CALL(*testIncDevice, filesystemType()).Times(2).WillRepeatedly(Return("ValidFileSystem"));

    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    mountMonitor->markMounts(mountPointVec);

    EXPECT_TRUE(waitForLog("Including 1 mount points in on-access scanning"));

    auto secondContent = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(secondContent, expectedFileSystemStr);
}

TEST_F(TestMountMonitor, TestfileSystemSetIsLimitedTo100Entries)
{
    std::string templateStr = "filesystem";
    std::set<std::string> input;

    for (int it=0; it<=(MountMonitor::TELEMETRY_FILE_SYSTEM_LIST_MAX + 10); it++)
    {
        auto inputStr = templateStr + std::to_string(it);
        input.emplace(inputStr, true);
    }

    ASSERT_GT(input.size(), MountMonitor::TELEMETRY_FILE_SYSTEM_LIST_MAX);
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);
    mountMonitor->addFileSystemToTelemetry(input);
    EXPECT_EQ(input.size(), MountMonitor::TELEMETRY_FILE_SYSTEM_LIST_MAX);
}

TEST_F(TestMountMonitor, TestFileSystemTelemetryCanHandleLongFileSystemNames)
{
    std::string longFileSystemName = "iamareallylongfilesystemtypewhoneedstobetestedintelemetry";
    std::set<std::string> input {
        longFileSystemName
    };

    auto expectedFileSystemStr =R"sophos({"file-system-types":["iamareallylongfilesystemtypewhoneedstobetestedintelemetry"]})sophos";

    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    mountMonitor->addFileSystemToTelemetry(input);
    auto content = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(content, expectedFileSystemStr);
}

TEST_F(TestMountMonitor, TestFileSystemTelemetryCanHandleNonAlphaNumericCharacters)
{
    auto nonAlphaNumericCharacters = R"sophos(\"#@><./?!£$%^&*(){{}}~:;`¬-_=+1234567890)sophos";
    std::set<std::string> input {
        nonAlphaNumericCharacters
    };
    PRINT(nonAlphaNumericCharacters);

    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    mountMonitor->addFileSystemToTelemetry(input);
    auto content = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::string ExpectedTelemetry{ R"sophos({"file-system-types":["\\\"#@><./?!£$%^&*(){{}}~:;`¬-_=+1234567890"]})sophos" };
    EXPECT_EQ(content, ExpectedTelemetry);
}

TEST_F(TestMountMonitor, TestMountIsExcludedAsItsOnExcludedList)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ASSERT_TRUE(FILE_SYSTEMS_TO_EXCLUDE.size() > 0);

    auto excludedFileSystem = FILE_SYSTEMS_TO_EXCLUDE.cbegin()->substr();
    PRINT(excludedFileSystem);
    std::string expectedMessage = "Mount point testmount using filesystem " + excludedFileSystem + " is not supported and will be excluded from scanning";

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_CALL(*excludedMount, filesystemType()).WillOnce(Return(excludedFileSystem));
    EXPECT_FALSE(mountMonitor->isIncludedFilesystemType(excludedMount));
    EXPECT_TRUE(waitForLog(expectedMessage));
}

TEST_F(TestMountMonitor, TestMountIsExcludedAsItsSpecial)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedMessage = "Mount point testmount is system and will be excluded from scanning";

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_CALL(*excludedMount, isSpecial()).WillOnce(Return(true));

    EXPECT_FALSE(mountMonitor->isIncludedFilesystemType(excludedMount));
    EXPECT_TRUE(waitForLog(expectedMessage));
}

TEST_F(TestMountMonitor, TestMountIsExcludedAsItDoesntHavePropertiesForScannableMount)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedMessage = "Mount point testmount has been excluded from scanning";

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_CALL(*excludedMount, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isSpecial()).WillOnce(Return(false));

    EXPECT_FALSE(mountMonitor->isIncludedFilesystemType(excludedMount));
    EXPECT_TRUE(waitForLog(expectedMessage));
}

TEST_F(TestMountMonitor, TestMountIsIncludedIfHardDisc)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_CALL(*excludedMount, isHardDisc()).WillOnce(Return(true));
    EXPECT_TRUE(mountMonitor->isIncludedFilesystemType(excludedMount));
}

TEST_F(TestMountMonitor, TestMountIsIncludedIfNetworkAndNotRemote)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    ASSERT_TRUE( m_config.excludeRemoteFiles == false);

    EXPECT_CALL(*excludedMount, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isNetwork()).WillOnce(Return(true));
    EXPECT_TRUE(mountMonitor->isIncludedFilesystemType(excludedMount));
}

TEST_F(TestMountMonitor, TestMountIsExcludedIfNetworkAndIsRemote)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    m_config.excludeRemoteFiles = true;

    EXPECT_CALL(*excludedMount, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isNetwork()).WillOnce(Return(true));
    EXPECT_CALL(*excludedMount, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isOptical()).WillOnce(Return(false));
    EXPECT_FALSE(mountMonitor->isIncludedFilesystemType(excludedMount));
}

TEST_F(TestMountMonitor, TestMountIsIncludedIfRemovable)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_CALL(*excludedMount, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isRemovable()).WillOnce(Return(true));
    EXPECT_TRUE(mountMonitor->isIncludedFilesystemType(excludedMount));
}

TEST_F(TestMountMonitor, TestMountIsIncludedIfOptical)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::shared_ptr<NiceMock<MockMountPoint>> excludedMount = std::make_shared<NiceMock<MockMountPoint>>();
    auto mountMonitor = std::make_shared<MountMonitor>(m_config, m_mockSysCallWrapper, m_mockFanotifyHandler, m_mockSysPathsFactory);

    EXPECT_CALL(*excludedMount, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*excludedMount, isOptical()).WillOnce(Return(true));
    EXPECT_TRUE(mountMonitor->isIncludedFilesystemType(excludedMount));
}