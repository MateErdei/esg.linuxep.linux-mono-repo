/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "MountMonitorMemoryAppenderUsingTests.h"

#include "avscanner/avscannerimpl/MockMountPoint.h"
#include "common/WaitForEvent.h"
#include "datatypes/MockSysCalls.h"

#include "common/ThreadRunner.h"
#include "datatypes/SystemCallWrapper.h"
#include "mount_monitor/mount_monitor/MountMonitor.h"

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
        }

        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCallWrapper;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
        struct pollfd m_fds[2]{};
        WaitForEvent m_serverWaitGuard;
    };

    /*
     * Simple thread runner, used to ensure threads are stopped before objects are destroyed.
     */
    class SimpleThreadRunner
    {
    public:
        explicit SimpleThreadRunner(Common::Threads::AbstractThread& thread)
            : m_thread(thread)
        {
            m_thread.start();
        }

        ~SimpleThreadRunner()
        {
            killThreads();
        }

        void killThreads()
        {
            m_thread.requestStop();
            m_thread.join();
        }

    private:
        Common::Threads::AbstractThread& m_thread;
    };
}

TEST_F(TestMountMonitor, TestGetIncludedMountpoints) // NOLINT
{
    OnAccessConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    std::shared_ptr<StrictMock<MockMountPoint>> localFixedDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillOnce(Return(true));

    std::shared_ptr<StrictMock<MockMountPoint>> networkDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillOnce(Return(true));

    std::shared_ptr<StrictMock<MockMountPoint>> opticalDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*opticalDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isOptical()).WillOnce(Return(true));

    std::shared_ptr<StrictMock<MockMountPoint>> removableDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*removableDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isRemovable()).WillOnce(Return(true));

    std::shared_ptr<StrictMock<MockMountPoint>> specialDevice = std::make_shared<StrictMock<MockMountPoint>>();
    EXPECT_CALL(*specialDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isSpecial()).WillOnce(Return(true));

    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(removableDevice);
    allMountpoints.push_back(specialDevice);

    EXPECT_CALL(*specialDevice, mountPoint()).WillOnce(Return("testSpecialMountPoint"));

    MountMonitor mountMonitor(config, m_sysCallWrapper);
    EXPECT_EQ(mountMonitor.getIncludedMountpoints(allMountpoints).size(), 4);
}

TEST_F(TestMountMonitor, TestPollOfProcMounts) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    OnAccessConfig config;
    config.m_scanHardDisc = true;
    config.m_scanNetwork = true;
    config.m_scanOptical = true;
    config.m_scanRemovable = true;

    m_fds[1].revents = POLLPRI;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(m_fds, m_fds+2), Return(1)))
        .WillOnce(DoAll(InvokeWithoutArgs(&m_serverWaitGuard, &WaitForEvent::onEventNoArgs), Return(-1)));
    MountMonitor mountMonitor(config, m_mockSysCallWrapper);
    int numMountPoints = mountMonitor.getIncludedMountpoints(mountMonitor.getAllMountpoints()).size();
    common::ThreadRunner mountMonitorThread(mountMonitor, "mountMonitor");

    std::stringstream logMsg1;
    logMsg1 << "Including " << numMountPoints << " mount points in on-access scanning";
    EXPECT_TRUE(waitForLog(logMsg1.str()));
    EXPECT_EQ(appenderCount(logMsg1.str()), 1);
    m_serverWaitGuard.wait();

    EXPECT_EQ(appenderCount(logMsg1.str()), 2);
}