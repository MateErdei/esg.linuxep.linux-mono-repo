/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "MockMountPoint.h"
#include "RecordingMockSocket.h"

#include "avscanner/avscannerimpl/NamedScanRunner.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace avscanner::avscannerimpl;
using ::testing::Return;
using ::testing::StrictMock;


class TestNamedScanRunner : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_expectedExclusions.emplace_back("/exclude1.txt");
        m_expectedExclusions.emplace_back("/exclude2/");
        m_expectedExclusions.emplace_back("/exclude3/*/*.txt");
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
    bool scanHardDisc = true;
    bool scanNetwork = true;
    bool scanOptical = true;
    bool scanRemovable = true;

    std::vector<std::string> expectedExclusions;
    expectedExclusions.push_back("/bin");
    expectedExclusions.push_back("/boot");
    expectedExclusions.push_back("/dev");
    expectedExclusions.push_back("/etc");
    expectedExclusions.push_back("/home");
    expectedExclusions.push_back("/lib32");
    expectedExclusions.push_back("/lib64");
    expectedExclusions.push_back("/lib");
    expectedExclusions.push_back("/lost+found");
    expectedExclusions.push_back("/media");
    expectedExclusions.push_back("/mnt");
    expectedExclusions.push_back("/oldTarFiles");
    expectedExclusions.push_back("/opt");
    expectedExclusions.push_back("/proc");
    expectedExclusions.push_back("/redist");
    expectedExclusions.push_back("/root");
    expectedExclusions.push_back("/run");
    expectedExclusions.push_back("/sbin");
    expectedExclusions.push_back("/snap");
    expectedExclusions.push_back("/srv");
    expectedExclusions.push_back("/sys");
    expectedExclusions.push_back("/usr");
    expectedExclusions.push_back("/vagrant");
    expectedExclusions.push_back("/var");

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            expectedExclusions,
            scanHardDisc,
            scanNetwork,
            scanOptical,
            scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    int origNumPaths = socket->m_paths.size();

    expectedExclusions.push_back("/tmp");
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            expectedExclusions,
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