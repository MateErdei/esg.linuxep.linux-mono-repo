/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "MockMountPoint.h"
#include "RecordingMockSocket.h"

#include "avscanner/avscannerimpl/NamedScanRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/LogInitializedTests.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

#include <capnp/message.h>

#include <fstream>

using namespace avscanner::avscannerimpl;
using namespace avscanner::mountinfo;
using ::testing::Return;
using ::testing::StrictMock;

namespace fs = sophos_filesystem;


class TestNamedScanRunner : public LogInitializedTests
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

    static std::vector<std::string> getAllOtherDirs(const std::string& includedDir)
    {
        std::vector<std::string> allOtherDirs;
        for(const auto& p: fs::directory_iterator("/"))
        {
            if (p != includedDir)
            {
                allOtherDirs.push_back(p.path().string() + "/");

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
    bool m_scanHardDisc = true;
    bool m_scanNetwork = false;
    bool m_scanOptical = false;
    bool m_scanRemovable = false;
};

TEST_F(TestNamedScanRunner, TestNamedScanConfigDeserialisation) // NOLINT
{
    m_scanHardDisc = false;
    m_scanNetwork = true;
    m_scanOptical = false;
    m_scanRemovable = true;

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    NamedScanConfig config = runner.getConfig();
    EXPECT_EQ(config.m_scanName, m_expectedScanName);
    for (const auto& excl : config.m_excludePaths)
    {
        bool matchingExclusion = false;
        for (const auto& expectedExcl : m_expectedExclusions)
        {
            if (excl.path() == expectedExcl)
            {
                matchingExclusion = true;
            }
        }
        EXPECT_TRUE(matchingExclusion);
    }
    EXPECT_EQ(config.m_scanHardDisc, m_scanHardDisc);
    EXPECT_EQ(config.m_scanNetwork, m_scanNetwork);
    EXPECT_EQ(config.m_scanOptical, m_scanOptical);
    EXPECT_EQ(config.m_scanRemovable, m_scanRemovable);
}

TEST_F(TestNamedScanRunner, TestGetIncludedMountpoints) // NOLINT
{
    m_scanHardDisc = true;
    m_scanNetwork = true;
    m_scanOptical = true;
    m_scanRemovable = true;

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

    ::capnp::MallocMessageBuilder message;
    avscanner::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(removableDevice);
    allMountpoints.push_back(specialDevice);

    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    EXPECT_EQ(runner.getIncludedMountpoints(allMountpoints).size(), 4);
}

TEST_F(TestNamedScanRunner, TestDuplicateMountPointsGetDeduplicated) // NOLINT
{
    using namespace avscanner::avscannerimpl;
    using namespace avscanner::mountinfo;
    class MockMountPoint : public IMountPoint
    {
    public:
        std::string m_mountPoint;

        explicit MockMountPoint(const fs::path& fakeMount)
                : m_mountPoint(fakeMount)
        {}

        [[nodiscard]] std::string device() const override
        {
            return std::__cxx11::string();
        }

        [[nodiscard]] std::string filesystemType() const override
        {
            return std::__cxx11::string();
        }

        [[nodiscard]] bool isHardDisc() const override
        {
            return true;
        }

        [[nodiscard]] bool isNetwork() const override
        {
            return false;
        }

        [[nodiscard]] bool isOptical() const override
        {
            return false;
        }

        [[nodiscard]] bool isRemovable() const override
        {
            return false;
        }

        [[nodiscard]] bool isSpecial() const override
        {
            return false;
        }

        [[nodiscard]] std::string mountPoint() const override
        {
            return m_mountPoint;
        }
    };

    class MockMountInfo : public IMountInfo
    {
    public:
        std::vector<std::shared_ptr<IMountPoint>> m_mountPoints;
        std::vector<std::shared_ptr<IMountPoint>> mountPoints() override
        {
            return m_mountPoints;
        }
    };

    m_scanHardDisc = true;
    m_scanNetwork = true;
    m_scanOptical = true;
    m_scanOptical = true;
    m_scanRemovable = true;

    fs::create_directories("/tmp/mount/point/");
    std::ofstream("/tmp/mount/point/file1.txt");

    std::shared_ptr<MockMountInfo> mountInfo;
    mountInfo.reset(new MockMountInfo());
    mountInfo->m_mountPoints.emplace_back(
            std::make_shared<MockMountPoint>("/tmp/mount/point/")
    );
    mountInfo->m_mountPoints.emplace_back(
            std::make_shared<MockMountPoint>("/tmp/mount/point/")
    );

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    auto socket = std::make_shared<RecordingMockSocket>();
    NamedScanRunner runner(scanConfigOut);

    runner.setMountInfo(mountInfo);
    runner.setSocket(socket);
    runner.run();

    EXPECT_EQ(socket->m_paths.size(), 1);
}

TEST_F(TestNamedScanRunner, TestExcludeByStem) // NOLINT
{
    std::string stemExclusion = "/tmp/";

    ::capnp::MallocMessageBuilder message;
    m_expectedScanName = "TestExcludeByStemScan1";
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    int origNumPaths = socket->m_paths.size();

    m_expectedExclusions.push_back(stemExclusion);
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    int newNumPaths = socket2->m_paths.size();
    EXPECT_LT(newNumPaths, origNumPaths);
}

TEST_F(TestNamedScanRunner, TestExcludeByFullPath) // NOLINT
{
    fs::path testDir = "/tmp/TestExcludeByFullPath";
    fs::path fullPathExcludedFile = testDir / "foo";
    fs::path fullPathIncludedFile = testDir / "foobar";

    fs::create_directory(testDir);
    std::ofstream excludedFile(fullPathExcludedFile);
    excludedFile << "This file will be excluded from the scan.";
    std::ofstream includedFile(fullPathIncludedFile);
    includedFile << "This file will be included in the scan.";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    bool excludedFileFoundBeforeExcluding = false;
    bool includedFileFoundBeforeExcluding = false;
    for (const auto& p : socket->m_paths)
    {
        if (p == fullPathExcludedFile)
        {
            excludedFileFoundBeforeExcluding = true;
        }
        if (p == fullPathIncludedFile)
        {
            includedFileFoundBeforeExcluding = true;
        }
    }
    EXPECT_TRUE(excludedFileFoundBeforeExcluding);
    EXPECT_TRUE(includedFileFoundBeforeExcluding);

    m_expectedExclusions.push_back(fullPathExcludedFile);
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    bool excludedFileFoundAfterExcluding = false;
    bool includedFileFoundAfterExcluding = false;
    for (const auto& p : socket2->m_paths)
    {
        if (p == fullPathExcludedFile)
        {
            excludedFileFoundAfterExcluding = true;
        }
        if (p == fullPathIncludedFile)
        {
            includedFileFoundAfterExcluding = true;
        }
    }
    EXPECT_FALSE(excludedFileFoundAfterExcluding);
    EXPECT_TRUE(includedFileFoundAfterExcluding);

    fs::remove_all(testDir);
}

TEST_F(TestNamedScanRunner, TestExcludeByGlob) // NOLINT
{
    fs::path testDir = "/tmp/TestExcludeByGlob";
    fs::path globExcludedFile = testDir / "foo.txt";
    fs::path globIncludedFile = testDir / "foo.log";
    fs::path globExcludedFile2 = testDir / "foo.1";
    fs::path globIncludedFile2 = testDir / "foo.";

    fs::create_directory(testDir);
    std::ofstream excludedFile(globExcludedFile);
    excludedFile << "This file will be excluded from the scan.";
    std::ofstream includedFile(globIncludedFile);
    includedFile << "This file will be included in the scan.";
    std::ofstream excludedFile2(globExcludedFile2);
    excludedFile2 << "This file will be excluded from the scan.";
    std::ofstream includedFile2(globIncludedFile2);
    includedFile2 << "This file will be included in the scan.";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    bool excludedFileFoundBeforeExcluding = false;
    bool includedFileFoundBeforeExcluding = false;
    for (const auto& p : socket->m_paths)
    {
        if (p == globExcludedFile || p == globExcludedFile2)
        {
            excludedFileFoundBeforeExcluding = true;
        }
        if (p == globIncludedFile || p == globIncludedFile2)
        {
            includedFileFoundBeforeExcluding = true;
        }
    }
    EXPECT_TRUE(excludedFileFoundBeforeExcluding);
    EXPECT_TRUE(includedFileFoundBeforeExcluding);

    m_expectedExclusions.push_back(testDir / "*.txt");
    m_expectedExclusions.push_back(testDir / "foo.?");
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    bool excludedFileFoundAfterExcluding = false;
    bool includedFileFoundAfterExcluding = false;
    for (const auto& p : socket2->m_paths)
    {
        if (p == globExcludedFile || p == globExcludedFile2)
        {
            excludedFileFoundAfterExcluding = true;
        }
        if (p == globIncludedFile || p == globIncludedFile2)
        {
            includedFileFoundAfterExcluding = true;
        }
    }
    EXPECT_FALSE(excludedFileFoundAfterExcluding);
    EXPECT_TRUE(includedFileFoundAfterExcluding);

    fs::remove_all(testDir);
}

TEST_F(TestNamedScanRunner, TestExcludeByFilename) // NOLINT
{
    fs::path testDir = "/tmp/TestExcludeByFilename";
    fs::path filenameExcludedFile = testDir / "bar" / "foo";
    fs::path filenameIncludedFile = testDir / "foo" / "bar";

    fs::create_directory(testDir);
    fs::create_directory(testDir / "foo");
    fs::create_directory(testDir / "bar");
    std::ofstream excludedFile(filenameExcludedFile);
    excludedFile << "This file will be excluded from the scan.";
    std::ofstream includedFile(filenameIncludedFile);
    includedFile << "This file will be included in the scan.";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    bool excludedFileFoundBeforeExcluding = false;
    bool includedFileFoundBeforeExcluding = false;
    for (const auto& p : socket->m_paths)
    {
        if (p == filenameExcludedFile)
        {
            excludedFileFoundBeforeExcluding = true;
        }
        if (p == filenameIncludedFile)
        {
            includedFileFoundBeforeExcluding = true;
        }
    }
    EXPECT_TRUE(excludedFileFoundBeforeExcluding);
    EXPECT_TRUE(includedFileFoundBeforeExcluding);

    m_expectedExclusions.emplace_back("foo");
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    bool excludedFileFoundAfterExcluding = false;
    bool includedFileFoundAfterExcluding = false;
    for (const auto& p : socket2->m_paths)
    {
        if (p == filenameExcludedFile)
        {
            excludedFileFoundAfterExcluding = true;
        }
        if (p == filenameIncludedFile)
        {
            includedFileFoundAfterExcluding = true;
        }
    }
    EXPECT_FALSE(excludedFileFoundAfterExcluding);
    EXPECT_TRUE(includedFileFoundAfterExcluding);

    fs::remove_all(testDir);
}
