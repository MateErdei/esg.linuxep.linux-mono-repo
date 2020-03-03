/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "MockMountPoint.h"
#include "RecordingMockSocket.h"

#include "avscanner/avscannerimpl/NamedScanRunner.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <fstream>

using namespace avscanner::avscannerimpl;
using ::testing::Return;
using ::testing::StrictMock;

namespace fs = sophos_filesystem;

class TestNamedScanRunner : public ::testing::Test
{
public:
    void SetUp() override
    {
        for(const auto& p: getAllOtherDirs("/tmp"))
        {
            m_expectedExclusions.push_back(p);
        }

        Common::ApplicationConfiguration::applicationConfiguration().setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, fs::current_path() );
    }

    std::vector<std::string> getAllOtherDirs(std::string includedDir)
    {
        std::vector<std::string> allOtherDirs;
        for(const auto& p: fs::directory_iterator("/"))
        {
            if (p != includedDir)
            {
                allOtherDirs.push_back(p.path());

            }
        }
        return allOtherDirs;
    }

    Sophos::ssplav::NamedScan::Reader createNamedScanConfig(
            ::capnp::MallocMessageBuilder& message,
            std::vector<std::string> expectedExclusions,
            bool scanHardDisc,
            bool scanNetwork,
            bool scanOptical,
            bool scanRemovable)
    {
        Sophos::ssplav::NamedScan::Builder scanConfigIn = message.initRoot<Sophos::ssplav::NamedScan>();
        scanConfigIn.setName(m_expectedScanName);
        auto exclusions = scanConfigIn.initExcludePaths(expectedExclusions.size());
        for (unsigned i=0; i < expectedExclusions.size(); i++)
        {
            exclusions.set(i, expectedExclusions[i]);
        }
        scanConfigIn.setScanHardDrives(scanHardDisc);
        scanConfigIn.setScanNetworkDrives(scanNetwork);
        scanConfigIn.setScanCDDVDDrives(scanOptical);
        scanConfigIn.setScanRemovableDrives(scanRemovable);

        Sophos::ssplav::NamedScan::Reader scanConfigOut = message.getRoot<Sophos::ssplav::NamedScan>();

        return scanConfigOut;
    }

    std::string m_expectedScanName = "testScan";
    std::vector<std::string> m_expectedExclusions;
};

TEST_F(TestNamedScanRunner, TestNamedScanConfigDeserialisation) // NOLINT
{
    bool scanHardDisc = false;
    bool scanNetwork = true;
    bool scanOptical = false;
    bool scanRemovable = true;

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            scanHardDisc,
            scanNetwork,
            scanOptical,
            scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    NamedScanConfig config = runner.getConfig();
    EXPECT_EQ(config.m_scanName, m_expectedScanName);
    EXPECT_EQ(config.m_excludePaths, m_expectedExclusions);
    EXPECT_EQ(config.m_scanHardDisc, scanHardDisc);
    EXPECT_EQ(config.m_scanNetwork, scanNetwork);
    EXPECT_EQ(config.m_scanOptical, scanOptical);
    EXPECT_EQ(config.m_scanRemovable, scanRemovable);
}

TEST_F(TestNamedScanRunner, TestGetIncludedMountpoints) // NOLINT
{
    bool scanHardDisc = true;
    bool scanNetwork = true;
    bool scanOptical = true;
    bool scanRemovable = true;

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> localFixedDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillOnce(Return(true));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> networkDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillOnce(Return(true));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> opticalDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*opticalDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isOptical()).WillOnce(Return(true));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> removableDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*removableDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isRemovable()).WillOnce(Return(true));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> specialDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*specialDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isSpecial()).WillOnce(Return(true));
    EXPECT_CALL(*specialDevice, mountPoint()).WillOnce(Return("/proc"));

    ::capnp::MallocMessageBuilder message;
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(removableDevice);
    allMountpoints.push_back(specialDevice);

    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            scanHardDisc,
            scanNetwork,
            scanOptical,
            scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    EXPECT_EQ(runner.getIncludedMountpoints(allMountpoints).size(), 4);
}

TEST_F(TestNamedScanRunner, TestExcludeByStem) // NOLINT
{
    std::string stemExclusion = "/tmp";

    bool scanHardDisc = true;
    bool scanNetwork = false;
    bool scanOptical = false;
    bool scanRemovable = false;

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            scanHardDisc,
            scanNetwork,
            scanOptical,
            scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    int origNumPaths = socket->m_paths.size();

    m_expectedExclusions.push_back(stemExclusion);
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            scanHardDisc,
            scanNetwork,
            scanOptical,
            scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    int newNumPaths = socket2->m_paths.size();
    EXPECT_LT(newNumPaths, origNumPaths);
}
